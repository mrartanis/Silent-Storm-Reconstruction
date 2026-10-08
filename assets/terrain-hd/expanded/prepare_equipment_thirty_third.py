from pathlib import Path
import json,hashlib,shutil,re
import numpy as np
from PIL import Image
B=Path(__file__).resolve().parent
ST='equipment-thirty-third'
CALL=[5387,5408,5438]
def save(n,v):(B/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def peaks(a,x0=0):return [x+x0 for x in range(1,len(a)-1) if a[x]>a[x-1] and a[x]>a[x+1]]
proof=json.loads((B/f'source-check-{ST}.json').read_text(encoding='utf-8'))
ids=[x['id'] for x in proof]
reason={3914:'Genuine painted multi-island atlas, ornate metal motifs and narrow handle repeat phase need exact complete detail proof; not a technical exclusion. Zero calls this scope.',5379:'Genuine red painted medicine map with source cross and weak label glyphs; exact tiny glyph content not established. Pending source glyph proof, zero calls.',5380:'Genuine brown/gray multi-region painted map, fine thin overlapping thread-like source strokes require detailed native count/phase proof before faithful generation. Zero calls.',5386:'Genuine paper/gray/brown box map with multiple dense tiny printed labels; glyph content uncertainty, zero calls.',5407:'Genuine dark matte narrow map with multiple notched contours/weak central marks; detailed source counts not established in this scope. Zero calls; not merely aspect exclusion.',5431:'Genuine red fine-checker matte surface; this scope tries different gray 5438 source, preserving 5431 as uncalled genuine pending instead of claiming unsuitable. Zero calls.',5433:'Genuine red/brown painted map with two source label rows of unresolved tiny glyphs; zero calls.',5435:'Genuine brown package map with weak stamp glyphs, folds and bands; tiny original print not established, zero calls.',6901:'Genuine gray/green multi-island painted tool with rotated source glyphs and small circular fields; exact label meaning/count needs proof, zero calls.'}
constraints=[]
for r in proof:
 i=r['id'];o=Image.open(B/f'original/{i}.png').convert('RGBA');a=np.asarray(o);g=a[:,:,:3].mean(2)
 prior=[p for p in B.rglob(f'{i}-*.png') if ST not in p.name and ('source' in p.name or 'support' in p.name) and 'raw' not in p.name]
 r['prior_source_only_artifact_examples']=[{'path':p.relative_to(B.parents[2]).as_posix(),'sha256':sha(p)} for p in sorted(prior)[:8]]
 r['readonly_view_novelty']='Repeated source-only audit; no previous raw/toolcalls/jobs permitted by fresh guards.' if prior else 'Fresh current audit, no assertion about first-ever historical viewing. No previous raw/toolcalls/jobs permitted by fresh guards.'
 r['source_only_reason']=reason.get(i,'Full native source paint/detail footprint inspected; exact original field and phase proof persisted before prompt. One faithful wholecanvas edit planned; original alpha/root scalar unchanged.')
 if i not in CALL:continue
 if i==5438:
  hp=B/f'generated/{i}-{ST}-source-rgb-support.png';o.convert('RGB').resize((1536,512),Image.Resampling.NEAREST).save(hp)
  r['helper_recipe'].update(operations='Native stored RGBA to opaque RGB; ENTIRE128x32 sourcecanvas NEAREST fullcanvas affine to1536x512(3:1). No crop/padding/rotate/UV fitting/unpremultiply/art inserted. SourceA independently restored by root.',helper_size=[1536,512],helper_bbox=[0,0,1536,512],inverse='ENTIRE raw LANCZOS inverse resize to original4x512x128; original sourceA LANCZOS separately; existing default mask padding/pure scalar by root.',affine_wholecanvas=True,source_aspect=4,helper_aspect=3)
 c={'id':i,'native_size':list(o.size),'actual_sourceA_extrema':list(o.getchannel('A').getextrema()),'source_RGBA_SHA256':hashlib.sha256(o.tobytes()).hexdigest(),'before_prompt_proof':'Executed and persisted original-only exact native source tonal matrices/field counts/phase before prompts or calls. Diagnostic domains NEVER crop input references or composite artistic source RGB. No special acceptance threshold; exact source patterns and local relative tone recorded, weak threshold change alone not proof of new components.','source_bitmap_fields':[]}
 if i==5387:
  boxes=[[5,2,13,10],[17,2,25,10],[27,2,35,10],[38,2,46,10],[5,15,13,23],[17,15,25,23],[27,15,35,23],[38,15,46,23],[50,19,64,32]]
  assert len(boxes)==9
  c['field_count']={'square_fields':8,'square_rows':2,'square_columns':4,'separate_lower_right_round_field':1}
  for n,b in enumerate(boxes):
   x0,y0,x1,y1=b;v=g[y0:y1,x0:x1]
   c['source_bitmap_fields'].append({'name':f'square field {n+1}' if n<8 else 'one clipped lower-right circular field','domain':b,'native_RGB':a[y0:y1,x0:x1,:3].tolist(),'mean_RGB':a[y0:y1,x0:x1,:3].mean((0,1)).tolist(),'luma_std':float(v.std())})
  c['whole_source_RGB_matrix']=a[:,:,:3].tolist()
 elif i==5408:
  boxes=[('one narrow left vertical light strip',[0,0,8,42]),('one upper-middle muted notched field',[7,9,32,35]),('one lower rounded brown field and source center',[1,37,30,64])]
  c['field_count']={'left_strip':1,'upper_notched_paintfield':1,'lower_round_field':1,'lower_center_weak_rectangle':1}
  for n,b in boxes:
   x0,y0,x1,y1=b;c['source_bitmap_fields'].append({'name':n,'domain':b,'native_RGB':a[y0:y1,x0:x1,:3].tolist(),'mean_RGB':a[y0:y1,x0:x1,:3].mean((0,1)).tolist()})
  c['original_alpha_each_native_row']=a[:,:,3].tolist()
  c['weak_center_native_RGB']=a[47:53,14:18,:3].tolist()
  c['source_shadow_note']='Source includes RGB0 and alpha0 contours, as well as dim positive-alpha paint; retain original fullA and useful dim RGB. Do not infer every black pixel to be a material hole.'
 elif i==5438:
  c['field_count']={'left_dark_grayshaded_strip':1,'middle_gray_fine_checker_field':1,'right_vertical_strip':1,'right_top_dark_octagonal_mark':1,'right_bottom_round_dark_mark':1}
  c['source_bitmap_fields']=[{'name':n,'domain':b} for n,b in [('left dimgray strip',[0,0,21,32]),('middle checker field',[21,0,116,32]),('right strip two source marks',[116,0,128,32])]]
  c['core_checker_domain']=[30,2,113,30]
  c['native_checker_phase_per_row']=[{'y':y,'x_strict_luma_maxima':peaks(g[y,29:114],29),'native_luma_samples':g[y,30:113].tolist()} for y in range(2,30)]
  assert all(len(z['x_strict_luma_maxima'])>35 for z in c['native_checker_phase_per_row'])
  c['pattern_note']='Native alternating one-pixel checker pattern, nearest pixel phase stored for EVERY core row including original irregular/noisy interruptions. No idealized new checker lattice or woven relief; keep original fine 1-native-pixel scale and muted gray variation.'
  c['whole_source_RGB_matrix']=a[:,:,:3].tolist()
 constraints.append(c)
save(f'pattern-constraints-{ST}.json',constraints)
save(f'scope-{ST}.json',{'ids':ids,'call_ids':CALL,'pending_uncalled_ids':[i for i in ids if i not in CALL],'selection_novelty':'Fresh uncalled guarded scope; prior source-only helpers/views honestly logged as re-audit, not first-ever source inspection.','source_entry_count_at_guard':2371,'queue_count_at_guard':2833})
save(f'source-check-{ST}.json',proof)
shutil.copyfile(B/f'source-check-{ST}.json',B/f'source-check-{ST}-before-call.json')
for name in ['save_equipment_thirty_second_call.py','preview_equipment_thirty_second.py','detail_equipment_thirty_second.py']:
 s=(B/name).read_text(encoding='utf-8').replace('thirty-second','thirty-third')
 s=re.sub(r'(?m)^IDS=.*$', 'IDS='+repr(CALL),s)
 (B/name.replace('thirty_second','thirty_third')).write_text(s,encoding='utf-8')
common=['Use case: precise-object-edit. Asset type: Silent Storm faithful HD equipment diffuse bitmap.', 'Input image 1 is the ENTIRE original stored RGB source canvas, exposed through only a nearest source-only enlargement. Edit this complete existing image into an extremely subtle finer original painted texture. Keep the entire reference canvas edge-to-edge, exactly the same normalized UV positions, source footprint, dimensions ratio, relative palette, pattern scale, soft edges and number of marks. This is a flat game texture, not a rendered object photograph.', 'Keep the original material and original low-contrast painted local hierarchy. Dim gray/brown washes remain dim soft paint, tiny pale strokes stay subdued. Keep black/shadow fields and all gaps as source. Do not turn source wash into etched scratches, mottled photo grain, raised rim, glowing rod, protruding bevel, metallic new hardware, rough rust, cloth weave or glossy white plate. No new objects, parts, folds, holes, marks, lettering, borders, lighting, vignette or background; no blur-based count reduction.', 'Output the full opaque RGB texture only, same wholecanvas composition as image 1. The original source alpha will be independently retained; do not redesign any boundaries or make a cutout. No padding, whitespace, zoom, recentering, crop or fitting isolated islands.']
specific={5387:'Exactly preserve EIGHT existing broad flat pale-gray square paintfields in TWO rows of FOUR at their original native positions, sizes, slightly differing tones, and ONE separate partially clipped round gray field at the lower-right. Keep the original tiny irregular mottled gray/brown/dark background marks and native step/phase; do not interpret the squares as 3D pills or make new blister cavities/rims. These broad square and circular surfaces stay as smooth/flat as source, with NO new grain or dents. Only very subtle source-linked refinement to already mottled painted areas; retain original number and scale, without idealizing the pixel pattern or adding regular dots.',5408:'Keep the ONE thin subdued vertical light strip along the left, the ONE irregular muted upper notched brown paintfield, and ONE soft brown round/oval lower field with its original weak central narrow rectangle. Retain EVERY original notch, bend, flat dark interior, stroke width and curved boundary from image 1, same native phase and contour. Do not invent tooth-like raised geometry, bright shiny rims, or newly embossed center. Existing pale upper edges and left-stroke small pale crossmarks remain gently painted and as weak as reference; lower rounded boundary remains soft shaded paint. No sharpened glint, scratches, threads, screws or new dots.',5438:'Image 1 is a source-only affine of the COMPLETE128x32 original to1536x512, wholecanvas3:1; generate the FULL3:1 image with the exact same normalized UV of this helper. The whole output will be inversely resized to original4x512x128. Keep ONE narrow dark soft-gray left strip, ONE wide muted gray fine checker middle field, and ONE narrow gray right strip with exactly TWO original dark markings: upper small dark octagonal opening-like bitmap and lower dark round marking. The middle has the exact original alternating one-native-pixel tiny checker phase across its whole width/height, including source irregular interruptions and original broad gentle gray patch variation. Copy its density, pitch and phase from image1, do not draw a new coarser grid/weave/mesh, diagonal raised lattice or white specular tips. Black marks stay black, existing gray outlines stay quiet, left gray wash remains soft; no added rings, holes, seams or metal highlights.'}
for i in CALL:
 p=B/f'generated/{i}-prompt.txt';assert not p.exists()
 p.write_bytes('\r\n\r\n'.join(common[:2]+[specific[i]]+common[2:]).encode('utf-8'))
print('BEFORE prompt constraints persisted for',CALL)
