"""Summarize packaged UE CSV captures; exclude menu warmup and finish transition."""
from pathlib import Path
import csv,json,statistics,sys,argparse
csv.field_size_limit(20*1024*1024)
ROOT=Path(__file__).resolve().parents[1]
FIELDS=['FrameTime','GameThreadTime','RenderThreadTime','GPUTime','RHIThreadTime','RHI/DrawCalls','RHI/PrimitivesDrawn','GPUMem/LocalUsedMB']
def capture(path):
 with path.open(newline='',encoding='utf-8-sig') as f:
  reader=csv.DictReader(f);rows=[]
  for row in reader:
   try:
    if float(row['FrameTime'])>0:rows.append(row)
   except (ValueError,KeyError,TypeError):pass
 # CSV begins before the two-second replay countdown. Use elapsed time rather
 # than a fixed frame count, so differing frame rates cover the same route.
 elapsed=0;active=[]
 for row in rows:
  elapsed+=float(row['FrameTime'])/1000
  if 3<=elapsed<=21:active.append(row)
 try:label=str(path.relative_to(ROOT))
 except ValueError:label=str(path)
 result={'file':label,'frames':len(active),'captured_seconds':elapsed}
 for field in FIELDS:
  values=[]
  for row in active:
   try:values.append(float(row[field]))
   except (ValueError,KeyError,TypeError):pass
  values.sort()
  if values:result[field]={'median':statistics.median(values),'p95':values[int((len(values)-1)*.95)],'p99':values[int((len(values)-1)*.99)]}
 return result
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('captures',nargs='*',type=Path)
 parser.add_argument('--output',type=Path,default=ROOT/'release-work/performance/measurements.json')
 args=parser.parse_args()
 paths=[p.resolve() for p in args.captures]
 if not paths:
  baseline=ROOT/'Build/OriginalSites/Windows/BMXCruiser/Saved/Profiling/CSV/Profile(20261101_195629).csv'
  if baseline.exists():paths.append(baseline)
  paths+=sorted((ROOT/'release-work/performance').glob('preset-*.csv'))
 if not paths:parser.error('Supply one or more Unreal CSV captures.')
 result=[capture(p) for p in paths]
 args.output.parent.mkdir(parents=True,exist_ok=True)
 args.output.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result,indent=2))
