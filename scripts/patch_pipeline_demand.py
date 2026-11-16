#!/usr/bin/env python3
"""Prioritize currently requested shader compiles; leave command workers FIFO."""
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve();project=Path(__file__).resolve().parents[1]
if root==project/'vendor/parallel-n64':raise SystemExit('Use a build copy')
d=root/'mupen64plus-video-paraLLEl/parallel-rdp/parallel-rdp'
p=d/'rdp_worker.h';s=p.read_text()
if 'rdp_worker_new_prioritized' not in s:
 anchor='/* Any thread. */';assert s.count(anchor)==1
 s=s.replace(anchor,'''/* Optional independent-work priority. Read before each item; it may change
 * while queued. Normal rdp_worker_new callers retain exact FIFO ordering. */
typedef uint64_t (*rdp_worker_priority_t)(void *user,const mpsc_stack_node_t *item);
rdp_worker_t *rdp_worker_new_prioritized(rdp_worker_start_t start,
      rdp_worker_work_t work,void *user,rdp_worker_priority_t priority);

'''+anchor);s=s.replace('#include <boolean.h>','#include <stdint.h>\n#include <boolean.h>');p.write_text(s)
p=d/'rdp_worker.c';s=p.read_text()
if 'rdp_worker_new_prioritized' not in s:
 s=s.replace('   rdp_worker_work_t  work;','   rdp_worker_work_t  work;\n   rdp_worker_priority_t priority;')
 old='''      while (n)
      {
         mpsc_stack_node_t *next = n->next;
         w->work(w->user, n);
         /* per item: a waiter may be waiting on this one */
         retro_eventcount_notify(&w->done_ec);
         n = next;
      }'''
 new='''      while (n)
      {
         mpsc_stack_node_t *item;
         if (w->priority)
         {
            /* A compile can take hundreds of ms. Reconsider recent demand
             * and arrivals after each job, rather than draining an old batch. */
            mpsc_stack_node_t *incoming=rdp_worker_reverse(mpsc_stack_drain(&w->queue));
            mpsc_stack_node_t **best=&n,**link;
            uint64_t score;
            if (incoming)
            {
               mpsc_stack_node_t *tail=n;
               while (tail->next) tail=tail->next;
               tail->next=incoming;
            }
            score=w->priority(w->user,*best);
            for (link=&n;*link;link=&(*link)->next)
            {
               uint64_t candidate=w->priority(w->user,*link);
               if (candidate>score) {best=link;score=candidate;}
            }
            item=*best;*best=item->next;item->next=NULL;
         }
         else {item=n;n=n->next;}
         w->work(w->user,item);
         retro_eventcount_notify(&w->done_ec);
      }'''
 assert s.count(old)==1;s=s.replace(old,new)
 anchor='''rdp_worker_t *rdp_worker_new(rdp_worker_start_t start,
      rdp_worker_work_t work, void *user)
{'''
 replacement='''rdp_worker_t *rdp_worker_new(rdp_worker_start_t start,
      rdp_worker_work_t work, void *user)
{
   return rdp_worker_new_prioritized(start,work,user,NULL);
}

rdp_worker_t *rdp_worker_new_prioritized(rdp_worker_start_t start,
      rdp_worker_work_t work,void *user,rdp_worker_priority_t priority)
{'''
 assert s.count(anchor)==1;s=s.replace(anchor,replacement)
 s=s.replace('   w->work  = work;','   w->work  = work;\n   w->priority=priority;');p.write_text(s)
p=d/'rdp_renderer.hpp';s=p.read_text()
if 'pipeline_priority' not in s:
 s=s.replace('#include <unordered_set>','#include <unordered_set>\n#include <unordered_map>\n#include <memory>\n#include <atomic>')
 old='\tstd::unordered_set<Util::Hash> pending_async_pipelines;';assert s.count(old)==1
 s=s.replace(old,'''    std::unordered_map<Util::Hash,std::shared_ptr<std::atomic<uint64_t>>> pending_async_pipelines;
    uint64_t pipeline_demand_serial=0;''')
 anchor='\tstatic void pipeline_work(void *user, mpsc_stack_node_t *item);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'\n    static uint64_t pipeline_priority(void *user,const mpsc_stack_node_t *item);');p.write_text(s)
p=d/'rdp_renderer.cpp';s=p.read_text()
if 'extern "C" double nps64_profile_time(void);' not in s:
 s='extern "C" double nps64_profile_time(void);\n'+s
 p.write_text(s)
if '[pipeline-demand]' not in s:
 s=s.replace('\tVulkan::DeferredPipelineCompile compile;\n};','\tVulkan::DeferredPipelineCompile compile;\n    std::shared_ptr<std::atomic<uint64_t>> demand;\n    double queued_at;\n};',1)
 s=s.replace('rdp_worker_new(nullptr, pipeline_work, device)','rdp_worker_new_prioritized(nullptr,pipeline_work,device,pipeline_priority)')
 a=s.index('\t\t\tif (pending_async_pipelines.count(compile.hash) == 0)');b=s.index('\n\t\t\tcmd.set_specialization_constant_mask(7);',a)
 s=s[:a]+'''            auto found=pending_async_pipelines.find(compile.hash);
            const uint64_t serial=++pipeline_demand_serial;
            if (found==pending_async_pipelines.end())
            {
                auto demand=std::make_shared<std::atomic<uint64_t>>(serial);
                pending_async_pipelines.emplace(compile.hash,demand);
                auto *node=new PipelineNode;
                node->compile=std::move(compile);node->demand=demand;
                node->queued_at=nps64_profile_time();
                LOGI("[pipeline-demand] enqueue specialized=%016llx policy=recent-demand\\n",
                     static_cast<unsigned long long>(node->compile.hash));
                rdp_worker_push(pipeline_worker->worker,node);
            }
            else found->second->store(serial,std::memory_order_relaxed);'''+s[b:]
 anchor='void Renderer::pipeline_work(void *user, mpsc_stack_node_t *item)\n{';assert s.count(anchor)==1
 s=s.replace(anchor,'''uint64_t Renderer::pipeline_priority(void *,const mpsc_stack_node_t *item)
{
    const auto *node=static_cast<const PipelineNode *>(item);
    return node->demand->load(std::memory_order_relaxed);
}

'''+anchor)
 anchor='\tauto *node = static_cast<PipelineNode *>(item);';assert s.count(anchor)==1
 s=s.replace(anchor,anchor+'''
    LOGI("[pipeline-demand] start specialized=%016llx queue_ms=%.3f\\n",
         static_cast<unsigned long long>(node->compile.hash),
         1000.0*(nps64_profile_time()-node->queued_at));''');p.write_text(s)
print('Demand-prioritized shader worker; render worker FIFO retained in',root)
