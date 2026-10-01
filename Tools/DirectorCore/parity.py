"""Replay independent reference Python gates through the native C++ protocol."""
import argparse,itertools,json,subprocess,sys
from dataclasses import replace
from pathlib import Path
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'../NarrativeCore'))
from director_runtime import Observation,Progress,scene_eligible,banter_eligible,banter_can_resume,_facts
from act12_rules import Actor,SCENES as EARLY,can_witness_j1
from act35_rules import SCENES as LATE,ROUTE_ENDS,can_witness_late_clue
from banter_rules import EXCHANGES
from variant_rules import scene_available,resolve_epilogue
MODES=('Easy','Normal','Hard','Punitive')

def quote(value):return json.dumps(value,ensure_ascii=False)
def group(values):return str(len(values))+' '+ ' '.join(quote(x) for x in sorted(values))
def encode(kind,id,o,p,outcomes=None):
    tokens=[kind,quote(id),str(MODES.index(o.difficulty)),str(o.act),quote(o.joe_node),str(o.now_seconds)]
    tokens+=list(map(lambda b:str(int(b)),(o.hunter_has_evidence,o.hunter_can_reach,o.alternative_reachable,o.area_previously_visited)))
    tokens += [quote(o.route_failure),str(int(o.in_danger)), '1',quote(p.active_scene),quote(p.active_banter),str(p.last_banter_completed_at)]
    tokens += [group(s) for s in (p.scenes,p.banter,p.witnessed_clues,p.durable_facts,o.facts)]
    tokens.append(str(len(o.companions)))
    for name,a in sorted(o.companions.items()):tokens += [quote(name),quote(a.node),str(int(a.active))]
    if outcomes:
        tokens.append('1')
        for name in ('bartholomeu','harley','jack'):
            tokens += [str(int(outcomes[name]==state)) for state in ('together','escaped_alone','captured','dead')]
    return ' '.join(tokens)
def actors(node):return {name:Actor(node) for name in ('thomas','harley','bartholomeu','jack')}
def verify(driver):
    rows=[];expected=[]
    def add(kind,id,o,p,result,outcomes=None):rows.append(encode(kind,id,o,p,outcomes));expected.append(result)
    seen=frozenset({'N01','N02','N03','N09','N25'})
    for id,s in {**EARLY,**LATE}.items():
        node=s['anchors'][0] if s['anchors'] else 'F3_CENTRAL'
        facts=set(s.get('facts_any',())) | {'joe_identified_thomas_body','joe_verified_batman_record','route_failure_observed','route_failed:roof','isolation_known','hide_victory','dawn_victory','rare_confrontation','rare_destruction'}
        for mode,act in itertools.product(MODES,range(1,6)):
            o=Observation(mode,act,node,actors(node),frozenset(facts),True,True,True,True,'roof')
            p=Progress(scenes=seen-{id})
            contexts=[(o,p),(replace(o,companions={}),p),(replace(o,joe_node='X_WINDOW'),p),(replace(o,facts=frozenset()),p),(o,Progress()),(replace(o,hunter_has_evidence=False,hunter_can_reach=False,alternative_reachable=False,area_previously_visited=False),p),(o,replace(p,active_scene='N01'))]
            for current,progress in contexts:add('scene',id,current,progress,str(int(scene_eligible(id,current,progress))))
    for id,(node,fact) in ROUTE_ENDS.items():
        for mode,with_fact in itertools.product(MODES,(False,True)):
            o=Observation(mode,1,node,facts=frozenset({fact}) if with_fact else frozenset());p=Progress(scenes=seen)
            add('scene',id+':exit',o,p,str(int(scene_eligible(id+':exit',o,p))))
    for id,s in EXCHANGES.items():
        node=s['nodes'][0]
        for mode in MODES:
            o=Observation(mode,s['act'],node,actors(node),frozenset(s.get('requires_facts',())))
            p=Progress(scenes=frozenset(s.get('after',())))
            contexts=[(o,p),(replace(o,in_danger=True),p),(replace(o,companions={}),p),(replace(o,joe_node='X_WINDOW'),p),(replace(o,act=s['act']%5+1),p),(o,replace(p,banter=frozenset({id}))),(o,replace(p,active_scene='N01')),(o,replace(p,last_banter_completed_at=0)),(replace(o,now_seconds=90),replace(p,last_banter_completed_at=0))]
            for current,progress in contexts:add('banter',id,current,progress,str(int(banter_eligible(id,current,progress))))
            active=replace(p,active_banter=id,active_banter_line=1)
            for current in (o,replace(o,in_danger=True),replace(o,companions={})):
                add('resume',id,current,active,str(int(banter_can_resume(current,active))))
    for id,mode,act,clues in itertools.product(('J1','J2','J3','J4'),MODES,range(1,6),(frozenset(),frozenset({'J1','J3'}),frozenset({'J1','J2'}))):
        o=Observation(mode,act,'F3_EXAMS',actors('F3_EXAMS'),frozenset({'jack_act3_encounter','jack_heard_thomas_or_injury','jack_perceived_companion_risk','jack_final_encounter'}))
        p=Progress(scenes=frozenset({'N08','N25'}),witnessed_clues=clues)
        facts=_facts(o,p)
        result=can_witness_j1(facts,True) and scene_available(id,mode,clues,act=act) if id=='J1' else can_witness_late_clue(id,facts,act,True)
        add('clue',id,o,p,str(int(result)))
        add('variant',id,o,p,str(int(scene_available(id,mode,clues,act=act))))
    for mode,id in itertools.product(MODES,[f'DOC{i:02d}' for i in range(1,17)]+[f'AUD{i:02d}' for i in range(1,9)]):
        allowed=not ((id in ('DOC16','AUD08') and mode in ('Easy','Normal')) or (id=='AUD07' and mode!='Punitive'))
        add('inspect',id,Observation(mode,1,'F4_START'),Progress(),str(int(allowed)))
    for mode,states in itertools.product(MODES,itertools.product(('together','escaped_alone','captured','dead','unknown'),repeat=3)):
        outcomes=dict(zip(('harley','bartholomeu','jack'),states))
        for thomas,jack in itertools.product((False,True),(False,True)):
            if thomas and mode in ('Easy','Normal') or jack and mode!='Punitive':continue
            p=Progress(scenes=frozenset({'N34'}) if thomas else frozenset(),witnessed_clues=frozenset({'J1','J3','J4'}) if jack else frozenset())
            values=resolve_epilogue(mode,outcomes,thomas,jack)
            result=' '.join(quote(values[name]) for name in ('bartholomeu','harley','jack','jack_secret','thomas'))+' '
            add('epilogue','end',Observation(mode,5,'F4_START'),p,result,outcomes)
    result=subprocess.run([str(driver)],input='\n'.join(rows)+'\n',text=True,capture_output=True,check=True).stdout.splitlines()
    assert len(result)==len(expected)
    for index,(got,want) in enumerate(zip(result,expected)):
        assert got==want,f'Native/reference mismatch at vector {index}: {rows[index].split()[0]}'
    print(f'DirectorParity: {len(rows)} Python/native vectors passed')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--driver',required=True);args=parser.parse_args();verify(Path(args.driver).resolve())
