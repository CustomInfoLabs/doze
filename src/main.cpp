#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <json.hpp>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <algorithm>
#include "power.hpp"
#include "embedded.hpp"
#include "version.hpp"
using namespace Gdiplus;
using json=nlohmann::json;
namespace fs=std::filesystem;
std::wstring wide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),w.data(),n);return w;}
std::wstring error_text(DWORD code){wchar_t* text=nullptr;FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,0,(LPWSTR)&text,0,nullptr);std::wstring result=text?text:L"Windows error";if(text)LocalFree(text);return result+L" ("+std::to_wstring(code)+L")";}
struct Hit {int id;RectF box;std::wstring label;bool enabled;};
enum class Modal {None,Settings,About,Message};
struct Palette {Color bg,well,line,text,muted,dim,blue,orange;};
class App {
public:
 HWND window=nullptr;HINSTANCE instance=nullptr;fs::path data,report;
 bool test=false,force=false,keyboard=false,dirty=true,interactionTest=false,toggleTest=false;
 std::unique_ptr<Bitmap> surface;
 ULONGLONG started=0,firstPaint=0,lastRemaining=~0ULL,lastCapsPoll=0;HANDLE helper=nullptr;
 json settings={{"language","auto"},{"theme","dark"},{"warning",true}};
 doze::Capabilities caps{};doze::Countdown countdown;doze::Action selected=doze::Action::Shutdown;
 int hours=0,minutes=30,seconds=0,hot=0,focus=0,testPhase=0;std::wstring edit;
 Modal modal=Modal::None;std::wstring messageTitle,messageBody,confirmLabel,toast;ULONGLONG toastUntil=0;
 std::function<void()> confirmed;
 std::vector<Hit> hits;std::vector<std::string> simulated;
 float width=580,height=840,dpi=1,bodyScale=1,bodyOffset=0;bool bodyDrawing=false;
 std::vector<std::vector<BYTE>> fontData;
 PrivateFontCollection displayFonts,boldFonts,bodyFonts,mediumFonts;std::unique_ptr<FontFamily> displayFamily,boldFamily,bodyFamily,mediumFamily;
 std::map<int,std::unique_ptr<Font>> fontCache;
 struct ToggleMotion {float from=0,target=0;ULONGLONG began=0;};
 std::map<int,ToggleMotion> toggleMotions;
 struct ButtonMotion {bool hovered=false;ULONGLONG began=0;};
 std::map<int,ButtonMotion> buttonMotions;
 bool motionAllowed=true;
 float motionValue(const ToggleMotion& m,ULONGLONG now)const{float t=std::clamp((float)(now-m.began)/180.f,0.f,1.f);t=t*t*(3.f-2.f*t);return m.from+(m.target-m.from)*t;}
 Color mix(Color a,Color b,float t)const{return Color((BYTE)(a.GetR()+(b.GetR()-a.GetR())*t),(BYTE)(a.GetG()+(b.GetG()-a.GetG())*t),(BYTE)(a.GetB()+(b.GetB()-a.GetB())*t));}
 void animate(){ULONGLONG now=GetTickCount64();bool moving=false;for(const auto& [id,m]:toggleMotions)if(m.from!=m.target&&now-m.began<180)moving=true;for(const auto& [id,m]:buttonMotions)if(m.hovered&&now-m.began<500)moving=true;invalidate();if(!moving)KillTimer(window,2);}

 bool italian()const{auto lang=settings.value("language",std::string("auto"));return lang=="it"||(lang=="auto"&&PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_ITALIAN);}
 std::wstring tr(const wchar_t* it,const wchar_t* en)const{return italian()?it:en;}
 bool light()const{return settings.value("theme",std::string("dark"))=="light";}
 Color hoverFill()const{return light()?Color(216,232,250):Color(22,35,50);}
 Palette palette()const{return light()?Palette{Color(255,255,255),Color(234,240,247),Color(160,176,194),Color(20,44,72),Color(64,91,121),Color(84,109,135),Color(28,101,176),Color(189,75,9)}:Palette{Color(16,21,29),Color(10,16,25),Color(46,55,69),Color(238,244,255),Color(148,165,186),Color(128,145,166),Color(101,181,255),Color(255,138,42)};}
 void invalidate(){dirty=true;if(window)InvalidateRect(window,nullptr,FALSE);}
 void save(){fs::create_directories(data);auto temp=data/L"settings.tmp";{std::ofstream out(temp,std::ios::binary);out<<settings.dump(2);if(!out)throw std::runtime_error("Unable to save settings");}if(!MoveFileExW(temp.c_str(),(data/L"settings.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Unable to save settings");invalidate();}
 void load(){fs::create_directories(data);try{std::ifstream in(data/L"settings.json");if(in){auto s=json::parse(in);std::string lang=s.value("language","auto"),theme=s.value("theme","dark");if(lang=="auto"||lang=="it"||lang=="en")settings["language"]=lang;if(theme=="dark"||theme=="light")settings["theme"]=theme;if(s.contains("warning")&&s["warning"].is_boolean())settings["warning"]=s["warning"];}}catch(...){settings={{"language","auto"},{"theme","dark"},{"warning",true}};}caps=doze::capabilities();}
 void loadFonts(){for(const auto& r:embeddedResources){HRSRC h=FindResourceW(instance,MAKEINTRESOURCEW(r.id),RT_RCDATA);if(!h)throw std::runtime_error("Missing font resource");DWORD n=SizeofResource(instance,h);auto raw=(BYTE*)LockResource(LoadResource(instance,h));fontData.emplace_back(raw,raw+n);auto& bytes=fontData.back();std::string name=r.name;auto& collection=name=="Inter-Medium.ttf"?mediumFonts:name.starts_with("Inter")?bodyFonts:name=="ChakraPetch-Bold.ttf"?boldFonts:displayFonts;if(collection.AddMemoryFont(bytes.data(),(INT)bytes.size())!=Ok)throw std::runtime_error("Unable to load embedded font");}auto family=[](PrivateFontCollection& collection){int n=0;auto f=std::make_unique<FontFamily>();collection.GetFamilies(1,f.get(),&n);if(n!=1)throw std::runtime_error("Embedded font unavailable");return f;};displayFamily=family(displayFonts);boldFamily=family(boldFonts);bodyFamily=family(bodyFonts);mediumFamily=family(mediumFonts);}
 Font* font(float size,bool display=false,bool bold=false){size=std::max(size,12.f/(bodyDrawing?bodyScale:1.f));int key=(int)(size*10)*4+(display?2:0)+(bold?1:0);auto& ptr=fontCache[key];if(!ptr){auto family=display?(bold?boldFamily.get():displayFamily.get()):(bold?mediumFamily.get():bodyFamily.get());int style=display&&bold?FontStyleBold:FontStyleRegular;ptr=std::make_unique<Font>(family,size,style,UnitPixel);if(ptr->GetLastStatus()!=Ok)throw std::runtime_error("Embedded font style unavailable");}return ptr.get();}
 void text(Graphics& g,const std::wstring& value,RectF r,float size,Color color,bool display=false,bool bold=false,StringAlignment align=StringAlignmentNear){SolidBrush brush(color);StringFormat fmt(StringFormat::GenericTypographic());fmt.SetAlignment(align);fmt.SetLineAlignment(StringAlignmentCenter);fmt.SetFormatFlags(StringFormatFlagsNoWrap);fmt.SetTrimming(StringTrimmingEllipsisCharacter);g.DrawString(value.c_str(),-1,font(size,display,bold),r,&fmt,&brush);}
 void paragraph(Graphics& g,const std::wstring& value,RectF r,float size,Color color){SolidBrush brush(color);StringFormat fmt;g.DrawString(value.c_str(),-1,font(size),r,&fmt,&brush);}
 // Inter Medium (500): slightly stronger text with explicit, restrained tracking.
 void readableText(Graphics& g,const std::wstring& value,RectF r,float size,Color color,float spacing=.2f,bool wrap=false){
  StringFormat fmt(StringFormat::GenericTypographic());fmt.SetFormatFlags(fmt.GetFormatFlags()|StringFormatFlagsMeasureTrailingSpaces);
  auto advance=[&](wchar_t ch){RectF measured;std::wstring glyph(1,ch);g.MeasureString(glyph.c_str(),1,font(size,false,true),PointF(0,0),&fmt,&measured);return measured.Width+spacing;};
  std::vector<std::wstring> rows;std::wstring row;float rowWidth=0;
  for(size_t i=0;i<value.size();){
   if(value[i]==L'\n'){rows.push_back(row);row.clear();rowWidth=0;i++;continue;}
   size_t end=value.find_first_of(L" \n",i);if(end==std::wstring::npos)end=value.size();
   std::wstring word=value.substr(i,end-i);float wordWidth=0;for(wchar_t ch:word)wordWidth+=advance(ch);
   float gap=row.empty()?0:advance(L' ');
   if(wrap&&!row.empty()&&rowWidth+gap+wordWidth>r.Width){rows.push_back(row);row.clear();rowWidth=0;gap=0;}
   if(!row.empty())row+=L' ';row+=word;rowWidth+=gap+wordWidth;
   i=end;if(i<value.size()&&value[i]==L' ')i++;
  }
  if(!row.empty())rows.push_back(row);
  float lineHeight=size*1.5f;float y=wrap?r.Y:r.Y+(r.Height-lineHeight)/2;
  for(const auto& line:rows){if(y+lineHeight>r.GetBottom()+1)break;float x=r.X;for(wchar_t ch:line){float step=advance(ch);if(x+step-spacing>r.GetRight()+1)break;text(g,std::wstring(1,ch),RectF(x,y,step+2,lineHeight),size,color,false,true);x+=step;}y+=lineHeight;}
 }
 void fill(Graphics& g,RectF r,Color c){SolidBrush b(c);g.FillRectangle(&b,r);}
 void gradient(Graphics& g,RectF r,Color from,Color to,float angle=140){LinearGradientBrush b(r,from,to,angle);g.FillRectangle(&b,r);}
 void tracked(Graphics& g,const std::wstring& value,RectF r,float size,Color color,float spacing=1.2f){float x=r.X;StringFormat fmt(StringFormat::GenericTypographic());fmt.SetFormatFlags(fmt.GetFormatFlags()|StringFormatFlagsMeasureTrailingSpaces);for(wchar_t ch:value){std::wstring s(1,ch);RectF measured;g.MeasureString(s.c_str(),1,font(size,true),PointF(0,0),&fmt,&measured);text(g,s,RectF(x,r.Y,measured.Width+2,r.Height),size,color,true);x+=measured.Width+spacing;}}
 void border(Graphics& g,RectF r,Color c,float thickness=1){Pen p(c,thickness);g.DrawRectangle(&p,r);}
 void line(Graphics& g,float x,float y,float x2,float y2,Color c,float thickness=1){Pen p(c,thickness);g.DrawLine(&p,x,y,x2,y2);}
 void logo(Graphics& g,float x,float y,float size){auto state=g.Save();g.TranslateTransform(x,y);g.ScaleTransform(size/64,size/64);SolidBrush orange(Color(255,138,42));g.FillRectangle(&orange,12.f,12.f,6.f,6.f);g.FillRectangle(&orange,12.f,25.f,6.f,27.f);Pen blue(Color(100,181,255),6);blue.SetLineJoin(LineJoinMiter);blue.SetStartCap(LineCapSquare);blue.SetEndCap(LineCapSquare);PointF pts[]={{25,15},{40,15},{49,24},{49,40},{40,49},{25,49}};g.DrawLines(&blue,pts,6);g.Restore(state);}
 void symbol(Graphics& g,int kind,float x,float y,float size,Color c){auto state=g.Save();g.SetPixelOffsetMode(PixelOffsetModeHalf);g.TranslateTransform(x,y);g.ScaleTransform(size/24,size/24);Pen pen(c,std::max(1.8f,1.35f/(size/24.f*(bodyDrawing?bodyScale:1.f))));pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);
  if(kind==0){g.DrawArc(&pen,4.f,4.f,16.f,16.f,310.f,280.f);g.DrawLine(&pen,12.f,2.f,12.f,11.f);}
  else if(kind==1){g.DrawArc(&pen,4.f,4.f,16.f,16.f,30.f,300.f);g.DrawLine(&pen,20.f,3.f,20.f,8.f);g.DrawLine(&pen,20.f,8.f,15.f,8.f);}
  else if(kind==2){GraphicsPath p;p.AddBezier(20.f,15.f,15.f,21.f,7.f,19.f,4.5f,13.f);p.AddBezier(4.5f,13.f,3.f,7.f,8.f,2.f,12.f,3.f);p.AddBezier(12.f,3.f,8.f,9.f,13.f,16.f,20.f,15.f);p.CloseFigure();g.DrawPath(&pen,&p);}
  else if(kind==3){g.DrawLine(&pen,3.f,6.f,11.f,6.f);g.DrawLine(&pen,11.f,6.f,3.f,14.f);g.DrawLine(&pen,3.f,14.f,11.f,14.f);g.DrawLine(&pen,14.f,3.f,20.f,3.f);g.DrawLine(&pen,20.f,3.f,14.f,9.f);g.DrawLine(&pen,14.f,9.f,20.f,9.f);}
  else if(kind==4){g.DrawEllipse(&pen,3.f,3.f,18.f,18.f);g.DrawLine(&pen,12.f,11.f,12.f,17.f);g.DrawLine(&pen,12.f,7.f,12.f,7.1f);}
  else if(kind==5){g.DrawLine(&pen,4.f,7.f,20.f,7.f);g.DrawLine(&pen,4.f,17.f,20.f,17.f);g.DrawLine(&pen,8.f,4.f,8.f,10.f);g.DrawLine(&pen,16.f,14.f,16.f,20.f);}
  else if(kind==6){g.DrawLine(&pen,6.f,6.f,18.f,18.f);g.DrawLine(&pen,18.f,6.f,6.f,18.f);}
  else if(kind==7){g.DrawLine(&pen,4.f,12.f,19.f,12.f);g.DrawLine(&pen,13.f,6.f,19.f,12.f);g.DrawLine(&pen,19.f,12.f,13.f,18.f);}
  else if(kind==8){g.DrawRectangle(&pen,5.f,10.f,14.f,11.f);g.DrawArc(&pen,8.f,3.f,8.f,10.f,180.f,180.f);g.DrawLine(&pen,12.f,15.f,12.f,17.f);}
  else{g.DrawLine(&pen,5.f,12.f,9.f,16.f);g.DrawLine(&pen,9.f,16.f,19.f,6.f);}g.Restore(state);
 }
 void hit(int id,RectF r,std::wstring label,bool enabled=true){if(bodyDrawing)r=RectF(bodyOffset+r.X*bodyScale,68+r.Y*bodyScale,r.Width*bodyScale,r.Height*bodyScale);hits.push_back({id,r,std::move(label),enabled});}
 void button(Graphics& g,int id,RectF r,const std::wstring& label,bool primary=false,bool enabled=true,int icon=-1){auto p=palette();Color bg=p.well;bool hovered=hot==id&&enabled;Color fg=primary?Color::White:(enabled?p.text:p.dim);GraphicsPath shape;PointF corners[]={{r.X,r.Y},{r.GetRight()-7,r.Y},{r.GetRight(),r.Y+7},{r.GetRight(),r.GetBottom()},{r.X+7,r.GetBottom()},{r.X,r.GetBottom()-7}};shape.AddPolygon(corners,6);if(primary){LinearGradientBrush brush(r,Color(198,83,11),Color(187,71,8),30);g.FillPath(&brush,&shape);auto& motion=buttonMotions[id];ULONGLONG now=GetTickCount64();if(hovered&&!motion.hovered){motion.began=now;if(motionAllowed&&window)SetTimer(window,2,16,nullptr);}motion.hovered=hovered;if(hovered&&motionAllowed&&now-motion.began<500){float progress=(float)(now-motion.began)/500.f;RectF shine(r.X-r.Width*1.1f+progress*r.Width*2.2f,r.Y,r.Width,r.Height);LinearGradientBrush highlight(shine,Color(0,255,255,255),Color(0,255,255,255),20);Color colors[]={Color(0,255,255,255),Color(0,255,255,255),Color(53,255,255,255),Color(0,255,255,255),Color(0,255,255,255)};REAL stops[]={0,.25f,.45f,.60f,1};highlight.SetInterpolationColors(colors,stops,5);auto clip=g.Save();g.SetClip(&shape);g.FillRectangle(&highlight,shine);g.Restore(clip);}}else{SolidBrush brush(light()?(enabled?Color(226,235,246):Color(237,241,246)):bg);g.FillPath(&brush,&shape);}Pen outline(primary?Color(0,0,0,0):hovered?p.blue:p.line);g.DrawPath(&outline,&shape);if(icon>=0){symbol(g,icon,r.X+14,r.Y+(r.Height-17)/2,17,fg);text(g,label,RectF(r.X+40,r.Y,r.Width-67,r.Height),13,fg,true,true);if(primary)symbol(g,7,r.GetRight()-32,r.Y+(r.Height-16)/2,16,fg);}else text(g,label,r,13,hovered&&!primary?p.blue:fg,true,true,StringAlignmentCenter);hit(id,r,label,enabled);if(focus==id&&keyboard&&enabled)border(g,RectF(r.X+3,r.Y+3,r.Width-6,r.Height-6),p.blue);}
 void textButton(Graphics& g,int id,RectF r,const std::wstring& label,int icon=-1){auto p=palette();float size=13;RectF bounds;StringFormat fmt(StringFormat::GenericTypographic());g.MeasureString(label.c_str(),-1,font(size),PointF(0,0),&fmt,&bounds);float groupWidth=std::min(r.Width,bounds.Width+(icon>=0?24.f:0.f));float left=(id==93||id==95||id==105)?r.GetRight()-groupWidth:r.X;text(g,label,RectF(left,r.Y,groupWidth-(icon>=0?24.f:0.f),r.Height),size,hot==id?p.text:p.blue);if(icon>=0)symbol(g,icon,left+groupWidth-16,r.Y+(r.Height-16)/2,16,p.blue);RectF target(left,r.Y,groupWidth,r.Height);hit(id,target,label);if(keyboard&&focus==id)border(g,target,p.blue);}
 void toggle(Graphics& g,int id,float x,float y,bool on,const std::wstring& label,bool enabled=true){auto p=palette();ULONGLONG now=GetTickCount64();float target=on?1.f:0.f;auto [it,inserted]=toggleMotions.try_emplace(id,ToggleMotion{target,target,now});auto& motion=it->second;if(motion.target!=target){motion.from=motionAllowed?motionValue(motion,now):target;motion.target=target;motion.began=now;if(motionAllowed&&window)SetTimer(window,2,16,nullptr);}float value=motionAllowed?motionValue(motion,now):target;RectF r(x,y,38,22);bool hovered=hot==id&&enabled;fill(g,r,hovered?hoverFill():mix(p.well,light()?Color(214,230,248):Color(24,40,58),value));border(g,r,hovered?p.blue:mix(p.line,p.blue,value));fill(g,RectF(x+4+16*value,y+4,13,14),enabled?(hovered?p.blue:mix(p.dim,p.blue,value)):p.line);hit(id,RectF(x-8,y-8,54,38),label,enabled);if(keyboard&&focus==id)border(g,RectF(x-3,y-3,44,28),p.blue);}
 std::wstring actionLabel(doze::Action a)const{switch(a){case doze::Action::Shutdown:return tr(L"Spegnimento",L"Shutdown");case doze::Action::Restart:return tr(L"Riavvio",L"Restart");case doze::Action::Sleep:return tr(L"Sospensione",L"Sleep");case doze::Action::Hibernate:return tr(L"Ibernazione",L"Hibernation");}return L"";}
 int duration()const{return hours*3600+minutes*60+seconds;}
 std::wstring durationText()const{int n=duration();if(!n)return tr(L"Imposta una durata per iniziare",L"Set a duration to start");std::wstring v=actionLabel(selected)+tr(L" tra ",L" in ");int h=n/3600,m=n%3600/60,s=n%60;std::vector<std::wstring> parts;if(h)parts.push_back(std::to_wstring(h)+(h==1?tr(L" ora",L" hour"):tr(L" ore",L" hours")));if(m)parts.push_back(std::to_wstring(m)+(m==1?tr(L" minuto",L" minute"):tr(L" minuti",L" minutes")));if(s)parts.push_back(std::to_wstring(s)+(s==1?tr(L" secondo",L" second"):tr(L" secondi",L" seconds")));for(size_t i=0;i<parts.size();i++){if(i)v+=tr(L" e ",L" and ");v+=parts[i];}return v;}
 void mainBody(Graphics& g){auto p=palette();bodyDrawing=true;auto state=g.Save();g.TranslateTransform(bodyOffset,68);g.ScaleTransform(bodyScale,bodyScale);
  tracked(g,tr(L"IL TUO PC. I TUOI TEMPI.",L"YOUR PC. YOUR TIME."),RectF(28,24,440,17),9,p.dim,1.8f);
  auto heading=tr(L"Quando vuoi,",L"On your terms,");RectF measured;StringFormat headingFormat(StringFormat::GenericTypographic());g.MeasureString(heading.c_str(),-1,font(25,true),PointF(0,0),&headingFormat,&measured);text(g,heading,RectF(28,44,measured.Width+4,36),25,p.text,true);text(g,tr(L"stacca.",L"switch off."),RectF(36+measured.Width,44,210,36),25,p.orange,true);
  text(g,wide(dozeVersion),RectF(475,45,77,25),10,p.dim,true,false,StringAlignmentFar);
  const wchar_t* namesIt[]={L"Spegni",L"Riavvia",L"Sospendi",L"Iberna"};const wchar_t* namesEn[]={L"Shut down",L"Restart",L"Sleep",L"Hibernate"};
  for(int i=0;i<4;i++){RectF r(28+i*132.f,98,124,82);bool chosen=(int)selected==i;if(chosen)gradient(g,r,light()?Color(220,234,250):Color(23,35,48),light()?Color(220,234,250):Color(17,24,33));else fill(g,r,hot==10+i?(light()?Color(221,234,249):Color(22,33,46)):p.well);border(g,r,chosen?(light()?p.blue:Color(64,94,126)):hot==10+i?p.blue:p.line);Color c=chosen?p.blue:(hot==10+i?p.text:p.muted);symbol(g,i,r.X+50,r.Y+16,24,c);text(g,italian()?namesIt[i]:namesEn[i],RectF(r.X,r.Y+49,r.Width,24),14,c,true,false,StringAlignmentCenter);hit(10+i,r,italian()?namesIt[i]:namesEn[i],!countdown.active);if(chosen)fill(g,RectF(r.X+50,r.GetBottom()-1,24,2),p.blue);if(keyboard&&focus==10+i)border(g,RectF(r.X+3,r.Y+3,r.Width-6,r.Height-6),p.blue);}
  RectF panel(28,202,520,282);fill(g,panel,p.well);if(light())gradient(g,panel,Color(237,244,252),Color(229,237,247),150);else{auto clip=g.Save();g.SetClip(panel);GraphicsPath ellipse;ellipse.AddEllipse(18.f,146.f,540.f,340.f);PathGradientBrush glow(&ellipse);glow.SetCenterColor(Color(21,34,48));Color surround=p.well;int count=1;glow.SetSurroundColors(&surround,&count);glow.SetCenterPoint(PointF(288,272));g.FillPath(&glow,&ellipse);g.Restore(clip);}border(g,panel,p.line);SolidBrush dot(p.blue);g.FillEllipse(&dot,48.f,225.f,5.f,5.f);
  text(g,countdown.active?tr(L"COUNTDOWN ATTIVO",L"COUNTDOWN RUNNING"):tr(L"PRONTO QUANDO VUOI",L"READY WHEN YOU ARE"),RectF(62,216,275,24),10,p.blue,true);
  text(g,actionLabel(selected),RectF(345,216,184,24),11,p.dim,true,false,StringAlignmentFar);
  ULONGLONG remaining=countdown.active?countdown.remaining(GetTickCount64()):(ULONGLONG)duration();int values[]={(int)(remaining/3600),(int)(remaining%3600/60),(int)(remaining%60)};bool final=countdown.active&&remaining<=10;Color digits=final?p.orange:(countdown.active?p.blue:p.text);
  for(int i=0;i<3;i++){wchar_t n[16];swprintf(n,16,L"%02d",values[i]);text(g,n,RectF(106+i*128.f,244,114,93),68,digits,true,false,StringAlignmentCenter);if(i<2)text(g,L":",RectF(217+i*128.f,248,22,82),45,light()?Color(85,139,194):Color(44,71,99),true,false,StringAlignmentCenter);}
  std::wstring note=countdown.active?(final&&settings.value("warning",true)?tr(L"Ultimi secondi. Puoi ancora annullare.",L"Final seconds. You can still cancel."):actionLabel(selected)+tr(L" programmato",L" scheduled")):durationText();text(g,note,RectF(48,338,480,27),13,p.muted,false,false,StringAlignmentCenter);
  fill(g,RectF(48,383,480,2),light()?Color(209,225,243):Color(20,38,58));if(countdown.active)fill(g,RectF(48,383,480.f*(1.f-(float)remaining/(float)countdown.seconds),2),digits);
  text(g,tr(L"DURATA",L"DURATION"),RectF(48,403,70,32),10,p.dim,true);
  int units[]={hours,minutes,seconds};for(int i=0;i<3;i++){float x=142+i*126.f;RectF r(x,400,116,38);if(hot==40+i*2)fill(g,RectF(x+1,401,28,36),hoverFill());if(hot==41+i*2)fill(g,RectF(x+87,401,28,36),hoverFill());border(g,r,focus==60+i?p.blue:p.line);Color minus=hot==40+i*2?p.blue:p.muted,plus=hot==41+i*2?p.blue:p.muted;line(g,x+10,419,x+20,419,minus,1.6f);line(g,x+96,419,x+106,419,plus,1.6f);line(g,x+101,414,x+101,424,plus,1.6f);wchar_t value[16];swprintf(value,16,L"%02d",units[i]);if(focus==60+i&&keyboard&&!countdown.active){fill(g,RectF(x+31,407,32,24),mix(p.well,p.blue,edit.empty()?.25f:.10f));if(!edit.empty())line(g,x+62,410,x+62,428,p.blue,1);}
   text(g,value,RectF(x+31,400,32,38),15,p.text,true,false,StringAlignmentCenter);text(g,i==0?L"h":i==1?L"m":L"s",RectF(x+65,400,16,38),12,p.dim,false,false,StringAlignmentCenter);hit(40+i*2,RectF(x+1,401,28,36),tr(L"Riduci ",L"Decrease ")+std::wstring(i==0?L"h":i==1?L"m":L"s"),!countdown.active);hit(41+i*2,RectF(x+87,401,28,36),tr(L"Aumenta ",L"Increase ")+std::wstring(i==0?L"h":i==1?L"m":L"s"),!countdown.active);hit(60+i,RectF(x+29,401,58,36),i==0?tr(L"Ore",L"Hours"):i==1?tr(L"Minuti",L"Minutes"):tr(L"Secondi",L"Seconds"),!countdown.active);}
  int preset[]={5,15,30,60,120};for(int i=0;i<5;i++){RectF r(204+i*65.f,446,62,24);std::wstring label=i<3?std::to_wstring(preset[i])+L" min":i==3?tr(L"1 ora",L"1 hour"):tr(L"2 ore",L"2 hours");bool active=duration()==preset[i]*60,hovered=hot==70+i&&!countdown.active;if(active||hovered){fill(g,r,hovered?hoverFill():(light()?Color(214,230,248):Color(15,29,45)));border(g,r,p.line);}text(g,label,r,11,active||hovered?p.blue:p.dim,true,false,StringAlignmentCenter);hit(70+i,r,label,!countdown.active);}
  bool forceAllowed=selected==doze::Action::Shutdown||selected==doze::Action::Restart;
  if(forceAllowed){text(g,tr(L"Forza chiusura programmi",L"Force apps to close"),RectF(28,505,445,24),14,p.text,false,true);text(g,tr(L"Può causare la perdita dei dati non salvati.",L"Unsaved work may be lost."),RectF(28,527,445,20),13,p.dim);toggle(g,92,514,514,force,tr(L"Forza chiusura programmi",L"Force apps to close"),!countdown.active);}
  bool unavailable=(selected==doze::Action::Hibernate&&!caps.hibernate)||(selected==doze::Action::Sleep&&!caps.sleep);Color status=unavailable?p.orange:p.blue;symbol(g,unavailable?3:9,28,560,15,status);text(g,unavailable?(selected==doze::Action::Hibernate?tr(L"Ibernazione non disponibile.",L"Hibernation unavailable."):tr(L"Sospensione non disponibile.",L"Sleep unavailable.")):tr(L"Il PC supporta questa azione.",L"Your PC supports this action."),RectF(50,553,400,26),13,p.muted);textButton(g,93,RectF(478,553,70,26),unavailable&&selected==doze::Action::Hibernate&&caps.hibernateSupported?tr(L"Attiva",L"Enable"):tr(L"Dettagli",L"Details"));
  button(g,91,RectF(28,598,100,44),tr(L"Annulla",L"Cancel"),false,countdown.active);button(g,90,RectF(138,598,410,44),tr(L"Avvia countdown",L"Start countdown"),true,!countdown.active&&!helper,(int)selected);
  symbol(g,8,28,666,14,p.muted);textButton(g,94,RectF(49,659,140,28),tr(L"Blocca PC",L"Lock PC"));textButton(g,95,RectF(244,659,304,28),tr(L"Annulla spegnimento Windows",L"Cancel Windows shutdown"),7);
  g.Restore(state);bodyDrawing=false;
 }
 void modalFrame(Graphics& g){
  if(modal==Modal::None)return;
  auto p=palette();Color divider=light()?Color(38,25,61,104):Color(19,255,255,255);
  SolidBrush veil(Color(190,3,7,13));g.FillRectangle(&veil,0.f,68.f,width,height-68);
  float w=(modal==Modal::Settings||modal==Modal::About)?480.f:470.f;
  float h=modal==Modal::Settings?380.f:modal==Modal::About?450.f:290.f;
  float x=(width-w)/2,y=(height-h)/2;
  fill(g,RectF(x,y,w,h),p.bg);border(g,RectF(x,y,w,h),p.blue);hits.clear();
  symbol(g,6,x+w-42,y+23,18,p.muted);hit(200,RectF(x+w-51,y+13,35,36),tr(L"Chiudi",L"Close"));
  if(modal==Modal::Settings){
   text(g,tr(L"PREFERENZE",L"PREFERENCES"),RectF(x+25,y+19,w-90,18),12,p.dim,true);
   text(g,tr(L"A modo tuo.",L"Make it yours."),RectF(x+25,y+44,w-90,35),25,p.text,true);
   const wchar_t* titleIt[]={L"Avviso prima dell’azione",L"Aspetto chiaro",L"Lingua",L"Ibernazione"};
   const wchar_t* titleEn[]={L"Warning before the action",L"Light appearance",L"Language",L"Hibernation"};
   float yy=y+96;
   for(int i=0;i<4;i++){
    float rowHeight=i==2?77.f:57.f;
    line(g,x+25,yy+rowHeight-8,x+w-25,yy+rowHeight-8,divider);
    float labelWidth=i==2?204.f:w-100;
    text(g,italian()?titleIt[i]:titleEn[i],RectF(x+25,yy,labelWidth,22),12,p.text,false,true);
    std::wstring info=i==0?tr(L"Un avviso negli ultimi 10 secondi.",L"A warning during the last 10 seconds."):i==1?tr(L"La stessa interfaccia, con più luce.",L"The same interface, in a brighter shade."):i==2?tr(L"Automatica dalla lingua di Windows.",L"Automatically follows Windows language."):caps.hibernate?tr(L"Attivata sul PC.",L"Enabled on your PC."):caps.hibernateSupported?tr(L"Disattivata sul PC.",L"Disabled on your PC."):tr(L"Non supportata su questo PC.",L"Not supported on this PC.");
    if(i==2)paragraph(g,info,RectF(x+25,yy+25,labelWidth,39),12,p.dim);
    else text(g,info,RectF(x+25,yy+23,w-50,20),12,p.dim);
    if(i<2)toggle(g,100+i,x+w-63,yy+8,i==0?settings.value("warning",true):light(),italian()?titleIt[i]:titleEn[i]);
    if(i==2){
     float widths[]={50,68,80};float left=x+w-25-210;
     for(int k=0;k<3;k++){
      RectF r(left,yy+18,widths[k],27);left+=widths[k]+6;
      bool chosen=settings.value("language","auto")==std::string(k==0?"auto":k==1?"it":"en");
      bool hovered=hot==102+k;
      fill(g,r,hovered?hoverFill():(chosen?mix(p.well,p.blue,.12f):p.well));border(g,r,chosen?p.blue:p.line);
      text(g,k==0?L"Auto":k==1?L"Italiano":L"English",r,12,chosen||hovered?p.blue:p.muted,true,false,StringAlignmentCenter);
      hit(102+k,r,k==0?tr(L"Lingua automatica",L"Automatic language"):k==1?L"Italiano":L"English");
     }
    }
    if(i==3)textButton(g,105,RectF(x+w-115,yy,90,25),helper?tr(L"In corso…",L"Working…"):caps.hibernate?tr(L"Disattiva",L"Disable"):tr(L"Attiva",L"Enable"));
    yy+=rowHeight;
   }
  }else if(modal==Modal::About){
   text(g,tr(L"INFORMAZIONI",L"ABOUT"),RectF(x+25,y+19,w-90,18),12,p.dim,true);
   fill(g,RectF(x+25,y+66,94,94),p.well);border(g,RectF(x+25,y+66,94,94),p.line);logo(g,x+36,y+77,72);
   text(g,L"Doze",RectF(x+143,y+63,119,54),42,p.text,true,true);text(g,L".",RectF(x+252,y+63,24,54),42,p.orange,true,true);
   readableText(g,tr(L"Il tuo PC. I tuoi tempi.",L"Your PC. Your time."),RectF(x+143,y+122,270,25),12,p.muted,.2f);
   readableText(g,tr(L"Programma spegnimento, riavvio, sospensione e ibernazione con un countdown semplice da controllare.",L"Schedule shutdown, restart, sleep and hibernation with an easy-to-control countdown."),RectF(x+25,y+183,w-50,49),13,p.muted,.2f,true);
   StringFormat fmt(StringFormat::GenericTypographic());
   for(int i=0;i<3;i++){
    float yy=y+246+i*33;line(g,x+25,yy-5,x+w-25,yy-5,divider);
    readableText(g,i==0?tr(L"Versione",L"Version"):i==1?tr(L"Realizzato da",L"Made by"):tr(L"Piattaforma",L"Platform"),RectF(x+25,yy,w-160,25),12,p.muted,.25f);
    if(i==0){
     auto stage=std::string(dozeVersion).starts_with("0.")?tr(L"IN SVILUPPO",L"IN DEVELOPMENT"):std::wstring(L"RELEASE");RectF measured;
     g.MeasureString(stage.c_str(),-1,font(12,true),PointF(0,0),&fmt,&measured);
     RectF badge(x+w-25-measured.Width-12,yy+2,measured.Width+12,22);
     fill(g,badge,mix(p.bg,p.blue,.05f));border(g,badge,p.line);
     text(g,stage,badge,12,p.blue,true,false,StringAlignmentCenter);
     text(g,wide(dozeVersion),RectF(x+160,yy,badge.X-x-170,25),12,p.text,true,false,StringAlignmentFar);
    }else if(i==1){
     RectF prefix,suffix;g.MeasureString(L"Custom",-1,font(12,true),PointF(0,0),&fmt,&prefix);g.MeasureString(L"Info",-1,font(12,true),PointF(0,0),&fmt,&suffix);
     float left=x+w-25-prefix.Width-suffix.Width;
     text(g,L"Custom",RectF(left,yy,prefix.Width,25),12,p.text,true);
     text(g,L"Info",RectF(left+prefix.Width,yy,suffix.Width,25),12,p.orange,true);
    }else text(g,L"Windows",RectF(x+160,yy,w-185,25),12,p.text,true,false,StringAlignmentFar);
   }
   RectF site(x+25,y+350,w-50,73);
   fill(g,site,hot==106?mix(p.bg,p.blue,.08f):p.well);border(g,site,hot==106?p.blue:p.line);
   tracked(g,tr(L"IL SITO UFFICIALE",L"OFFICIAL WEBSITE"),RectF(x+42,y+359,w-90,19),12,p.dim);
   text(g,L"custominfo.it",RectF(x+42,y+382,w-90,28),17,p.blue,true);symbol(g,7,x+w-60,y+377,20,p.blue);
   hit(106,site,L"custominfo.it");if(keyboard&&focus==106)border(g,site,p.blue);
  }else{
   text(g,messageTitle,RectF(x+25,y+36,w-75,43),24,p.text,true);paragraph(g,messageBody,RectF(x+25,y+99,w-50,102),12,p.muted);
   if(confirmLabel.empty())button(g,201,RectF(x+w-125,y+h-63,100,40),tr(L"Chiudi",L"Close"));
   else{button(g,201,RectF(x+25,y+h-63,100,40),tr(L"Annulla",L"Cancel"));button(g,202,RectF(x+135,y+h-63,w-160,40),confirmLabel,true);}
  }
 }
 void render(Graphics& g,float w,float h){width=w;height=h;bodyScale=std::min(1.f,(height-110.f)/690.f);bodyOffset=(width-580.f*bodyScale)/2;hits.clear();auto p=palette();g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetPixelOffsetMode(PixelOffsetModeNone);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.Clear(p.bg);border(g,RectF(.5f,.5f,width-1,height-1),p.line);fill(g,RectF(28,0,50,2),p.orange);line(g,0,67,width,67,p.line);logo(g,24,15,38);text(g,L"doze",RectF(73,16,78,40),29,p.text,true,true);text(g,L".",RectF(137,16,15,40),29,p.orange,true,true);symbol(g,4,width-180,24,19,hot==1?p.blue:p.muted);hit(1,RectF(width-190,15,36,37),tr(L"Informazioni su Doze",L"About Doze"));symbol(g,5,width-143,24,19,hot==2?p.blue:p.muted);hit(2,RectF(width-153,15,36,37),tr(L"Impostazioni",L"Settings"));line(g,width-105,25,width-105,42,p.line);line(g,width-80,32,width-68,32,p.muted);hit(3,RectF(width-92,15,36,37),tr(L"Riduci finestra",L"Minimize window"));symbol(g,6,width-43,24,18,hot==4?p.orange:p.muted);hit(4,RectF(width-52,15,36,37),tr(L"Chiudi Doze",L"Close Doze"));mainBody(g);line(g,0,height-42,width,height-42,p.line);text(g,L"by",RectF(28,height-37,20,30),12,p.dim);RectF brandBounds;StringFormat brandFormat(StringFormat::GenericTypographic());g.MeasureString(L"Custom",-1,font(13,true),PointF(0,0),&brandFormat,&brandBounds);text(g,L"Custom",RectF(52,height-37,brandBounds.Width,30),13,p.muted,true);text(g,L"Info",RectF(52+brandBounds.Width,height-37,40,30),13,p.orange,true);text(g,countdown.active?tr(L"Puoi annullare in ogni momento",L"You can cancel at any time"):test?L"TEST · No power actions":tr(L"Solo sul tuo PC",L"Only on your PC"),RectF(225,height-37,width-253,30),9,p.dim,false,false,StringAlignmentFar);modalFrame(g);if(!toast.empty()&&GetTickCount64()<toastUntil){fill(g,RectF(28,height-97,width-56,43),p.well);line(g,28,height-97,28,height-54,p.blue,2);text(g,toast,RectF(43,height-97,width-86,43),11,p.text);}}
 void prepareFrame(){RECT r;GetClientRect(window,&r);if(r.right<=0||r.bottom<=0)return;dpi=GetDpiForWindow(window)/96.f;if(!surface||surface->GetWidth()!=(UINT)r.right||surface->GetHeight()!=(UINT)r.bottom){surface=std::make_unique<Bitmap>(r.right,r.bottom,PixelFormat32bppRGB);dirty=true;}if(dirty){Graphics g(surface.get());g.ScaleTransform(dpi,dpi);render(g,r.right/dpi,r.bottom/dpi);dirty=false;if(!firstPaint)firstPaint=GetTickCount64();}}
 void paint(){PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);prepareFrame();if(surface){Graphics target(dc);target.SetCompositingMode(CompositingModeSourceCopy);target.DrawImage(surface.get(),Rect(0,0,surface->GetWidth(),surface->GetHeight()),0,0,surface->GetWidth(),surface->GetHeight(),UnitPixel);}EndPaint(window,&ps);}

 int at(float x,float y)const{for(auto it=hits.rbegin();it!=hits.rend();++it)if(it->enabled&&it->box.Contains(x,y))return it->id;return 0;}
 void notify(std::wstring value){toast=std::move(value);toastUntil=GetTickCount64()+3500;invalidate();}
 void dialog(std::wstring title,std::wstring body,std::wstring label=L"",std::function<void()> callback={}){modal=Modal::Message;messageTitle=std::move(title);messageBody=std::move(body);confirmLabel=std::move(label);confirmed=std::move(callback);focus=confirmLabel.empty()?201:202;invalidate();}
 void capabilitiesDialog(){dialog(tr(L"Stato energetico.",L"Power capabilities."),tr(L"Sospensione: ",L"Sleep: ")+(caps.sleep?tr(L"disponibile",L"available"):tr(L"non disponibile",L"unavailable"))+(caps.modernStandby?L" · Modern Standby":L"")+L"\n"+tr(L"Ibernazione: ",L"Hibernation: ")+(caps.hibernate?tr(L"disponibile",L"available"):caps.hibernateSupported?tr(L"disattivata o limitata dalle impostazioni",L"disabled or restricted by settings"):tr(L"non supportata",L"not supported"))+L"\n\n"+tr(L"Doze rispetta le impostazioni energetiche di Windows.",L"Doze follows Windows power settings."));}
 void configure(bool enable){if(helper||countdown.active){notify(tr(L"Annulla prima il countdown o attendi l’operazione.",L"Cancel the countdown or wait for the operation."));return;}if(test){notify(tr(L"Modifiche al sistema bloccate nel test.",L"System changes are disabled in test mode."));return;}wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);SHELLEXECUTEINFOW e{};e.cbSize=sizeof(e);e.hwnd=window;e.fMask=SEE_MASK_NOCLOSEPROCESS;e.lpVerb=L"runas";e.lpFile=exe;e.lpParameters=enable?L"--configure-hibernation=enable":L"--configure-hibernation=disable";e.nShow=SW_HIDE;if(!ShellExecuteExW(&e)){notify(error_text(GetLastError()));return;}helper=e.hProcess;invalidate();}
 void askHibernate(){if(!caps.hibernateSupported){capabilitiesDialog();return;}bool enable=!caps.hibernate;dialog(enable?tr(L"Attivare l’ibernazione?",L"Enable hibernation?"):tr(L"Disattivare l’ibernazione?",L"Disable hibernation?"),enable?tr(L"Windows chiederà l’autorizzazione di amministratore. Doze abiliterà l’ibernazione completa e ne controllerà la disponibilità. Il countdown non partirà da solo.",L"Windows will ask for administrator permission. Doze will enable full hibernation and check availability. The countdown will not start automatically."):tr(L"Questa modifica può influire anche sull’avvio rapido e sulla sospensione ibrida di Windows. Richiede l’autorizzazione di amministratore.",L"This can also affect Windows Fast Startup and hybrid sleep. Administrator permission is required."),enable?tr(L"Attiva",L"Enable"):tr(L"Disattiva",L"Disable"),[this,enable]{configure(enable);});}
 void start(){if(countdown.active||helper)return;caps=doze::capabilities();if(selected==doze::Action::Hibernate&&!caps.hibernate){askHibernate();return;}if(selected==doze::Action::Sleep&&!caps.sleep){capabilitiesDialog();return;}if(!duration()){notify(tr(L"Imposta una durata maggiore di zero.",L"Set a duration greater than zero."));return;}countdown.start(selected,duration(),force&&(selected==doze::Action::Shutdown||selected==doze::Action::Restart),GetTickCount64());lastRemaining=~0ULL;invalidate();}
 void close(){if(countdown.active)dialog(tr(L"Chiudere Doze?",L"Close Doze?"),tr(L"Chiudendo l’app verrà annullato il countdown in corso.",L"Closing the app will cancel the active countdown."),tr(L"Annulla countdown",L"Cancel countdown"),[this]{countdown.cancel();PostMessageW(window,WM_APP+1,0,0);});else PostMessageW(window,WM_APP+1,0,0);}
 void activate(int id){if(countdown.active&&((id>=10&&id<=13)||(id>=40&&id<=45)||(id>=60&&id<=62)||(id>=70&&id<=74)||id==90||id==92))return;try{
  if(id>=10&&id<=13){selected=(doze::Action)(id-10);edit.clear();}
  else if(id>=40&&id<=45){int unit=(id-40)/2;int& value=unit==0?hours:unit==1?minutes:seconds;value=std::clamp(value+((id%2)?1:-1),0,unit==0?99:59);}
  else if(id>=60&&id<=62){focus=id;edit.clear();keyboard=true;SetFocus(window);}
  else if(id>=70&&id<=74){int presets[]={5,15,30,60,120};int m=presets[id-70];hours=m/60;minutes=m%60;seconds=0;}
  else switch(id){
   case 1:modal=Modal::About;focus=200;break;case 2:modal=Modal::Settings;focus=200;break;case 3:ShowWindow(window,SW_MINIMIZE);break;case 4:close();break;
   case 90:start();break;case 91:countdown.cancel();notify(tr(L"Countdown annullato.",L"Countdown cancelled."));break;case 92:force=!force;break;case 93:if(selected==doze::Action::Hibernate&&!caps.hibernate&&caps.hibernateSupported)askHibernate();else capabilitiesDialog();break;
   case 94:if(test)simulated.push_back("lock");else if(!LockWorkStation())notify(error_text(GetLastError()));break;
   case 95:dialog(tr(L"Annulla spegnimento Windows.",L"Cancel Windows shutdown."),tr(L"Annullare uno spegnimento o riavvio già pianificato da Windows? Il countdown di Doze è separato.",L"Cancel a shutdown or restart already scheduled by Windows? The Doze countdown is separate."),tr(L"Annulla spegnimento",L"Cancel shutdown"),[this]{if(test){simulated.push_back("abort");return;}DWORD e=doze::shutdown_privilege();if(!e&&!AbortSystemShutdownW(nullptr))e=GetLastError();notify(e?error_text(e):tr(L"Spegnimento Windows annullato.",L"Windows shutdown cancelled."));});break;
   case 100:settings["warning"]=!settings.value("warning",true);save();break;case 101:settings["theme"]=light()?"dark":"light";save();break;
   case 102:case 103:case 104:settings["language"]=id==102?"auto":id==103?"it":"en";save();break;
   case 105:askHibernate();break;case 106:if(!test)ShellExecuteW(window,L"open",L"https://custominfo.it/",nullptr,nullptr,SW_SHOWNORMAL);break;
   case 200:case 201:modal=Modal::None;confirmed={};focus=0;break;
   case 202:{auto fn=std::move(confirmed);modal=Modal::None;focus=0;if(fn)fn();break;}
  }
  invalidate();
 }catch(const std::exception& e){notify(wide(e.what()));}}
 void key(WPARAM key){keyboard=true;if(key==VK_ESCAPE){if(modal!=Modal::None){modal=Modal::None;confirmed={};focus=0;}else if(countdown.active){countdown.cancel();notify(tr(L"Countdown annullato.",L"Countdown cancelled."));}invalidate();return;}
  if(key==VK_TAB){std::vector<int> order;for(auto& h:hits)if(h.enabled)order.push_back(h.id);if(order.empty())return;auto pos=std::find(order.begin(),order.end(),focus);int n=pos==order.end()?-1:(int)(pos-order.begin());n=(n+((GetKeyState(VK_SHIFT)&0x8000)?-1:1)+(int)order.size())%(int)order.size();focus=order[n];edit.clear();NotifyWinEvent(EVENT_OBJECT_FOCUS,window,OBJID_CLIENT,n+1);invalidate();return;}
  if(focus>=60&&focus<=62&&!countdown.active&&(key==VK_UP||key==VK_DOWN)){int u=focus-60;int& v=u==0?hours:u==1?minutes:seconds;v=std::clamp(v+(key==VK_UP?1:-1),0,u==0?99:59);edit.clear();invalidate();return;}
  if(key==VK_RETURN||key==VK_SPACE){if(focus&&std::any_of(hits.begin(),hits.end(),[this](const Hit& h){return h.id==focus&&h.enabled;}))activate(focus);}
 }
 void character(WPARAM value){if(countdown.active||focus<60||focus>62)return;if(value==8){if(!edit.empty())edit.pop_back();}else if(value>=L'0'&&value<=L'9'){if(edit.size()>=2)return;edit.push_back((wchar_t)value);}else return;int n=edit.empty()?0:std::stoi(edit);int u=focus-60;int& current=u==0?hours:u==1?minutes:seconds;current=std::min(n,u==0?99:59);invalidate();}
 void timer(){try{if(GetTickCount64()-lastCapsPoll>=2000){lastCapsPoll=GetTickCount64();auto fresh=doze::capabilities();if(fresh.sleep!=caps.sleep||fresh.hibernate!=caps.hibernate||fresh.hibernateSupported!=caps.hibernateSupported||fresh.filePresent!=caps.filePresent||fresh.modernStandby!=caps.modernStandby){caps=fresh;invalidate();}}if(helper&&WaitForSingleObject(helper,0)==WAIT_OBJECT_0){DWORD code;GetExitCodeProcess(helper,&code);CloseHandle(helper);helper=nullptr;caps=doze::capabilities();notify(code?tr(L"Impostazioni energetiche non aggiornate. Codice: ",L"Power settings were not updated. Exit code: ")+std::to_wstring(code):tr(L"Impostazioni energetiche aggiornate.",L"Power settings updated."));}auto remaining=countdown.remaining(GetTickCount64());if(countdown.due(GetTickCount64())){DWORD e=0;caps=doze::capabilities();if(selected==doze::Action::Sleep&&!caps.sleep)e=ERROR_NOT_SUPPORTED;else if(selected==doze::Action::Hibernate&&!caps.hibernate)e=ERROR_NOT_SUPPORTED;else if(test)simulated.push_back(selected==doze::Action::Restart?"restart":selected==doze::Action::Shutdown?"shutdown":selected==doze::Action::Sleep?"sleep":"hibernate");else e=doze::execute(countdown.action,countdown.force);dialog(e?tr(L"Operazione non riuscita.",L"Action failed."):tr(L"Countdown completato.",L"Countdown complete."),e?error_text(e):test?tr(L"Azione simulata. Nessun comando inviato a Windows.",L"Action simulated. No command sent to Windows."):tr(L"Azione richiesta a Windows.",L"Action requested from Windows."));}if(remaining!=lastRemaining){lastRemaining=remaining;invalidate();}if(!toast.empty()&&GetTickCount64()>=toastUntil){toast.clear();invalidate();}if(!report.empty())smoke();}catch(const std::exception& e){notify(wide(e.what()));}}
 bool snapshot(const fs::path& file,float scale=1){Bitmap bmp((INT)(580*scale),(INT)(840*scale),PixelFormat32bppRGB);Graphics g(&bmp);g.ScaleTransform(scale,scale);render(g,580,840);CLSID encoder;UINT count,size;GetImageEncodersSize(&count,&size);std::vector<BYTE> bytes(size);auto enc=(ImageCodecInfo*)bytes.data();GetImageEncoders(count,size,enc);for(UINT i=0;i<count;i++)if(wcscmp(enc[i].MimeType,L"image/png")==0){encoder=enc[i].Clsid;return bmp.Save(file.c_str(),&encoder,nullptr)==Ok;}return false;}
 void smoke(){if(testPhase==0){
  settings["language"]="it";settings["theme"]="dark";save();modal=Modal::None;
  activate(60);character(L'1');character(L'2');bool typed=hours==12;key(VK_UP);typed=typed&&hours==13;
  activate(72);selected=doze::Action::Shutdown;force=false;activate(90);activate(11);activate(92);
  bool protectedAction=countdown.active&&selected==doze::Action::Shutdown&&!force;
  activate(91);bool cancelled=!countdown.active;toast.clear();focus=0;keyboard=false;
  activate(2);activate(104);activate(101);bool preferences=light()&&!italian();
  fs::create_directories(report.parent_path());bool image=snapshot(report.parent_path()/L"settings.png");
  modal=Modal::None;image=image&&snapshot(report.parent_path()/L"light.png");activate(1);image=image&&snapshot(report.parent_path()/L"about.png");
  modal=Modal::None;settings["language"]="it";settings["theme"]="dark";save();image=image&&snapshot(report.parent_path()/L"dark.png");image=image&&snapshot(report.parent_path()/L"dark-125.png",1.25f);image=image&&snapshot(report.parent_path()/L"dark-150.png",1.5f);activate(2);image=image&&snapshot(report.parent_path()/L"settings-dark.png");activate(200);activate(1);image=image&&snapshot(report.parent_path()/L"about-dark.png");activate(200);
  activate(2);image=image&&snapshot(report.parent_path()/L"toggle-before.png");bool warning=settings.value("warning",true);activate(100);image=image&&snapshot(report.parent_path()/L"toggle-start.png");auto& motion=toggleMotions.at(100);if(motionAllowed){motion.began=GetTickCount64()-90;float middle=motionValue(motion,GetTickCount64());toggleTest=middle>.35f&&middle<.65f;image=image&&snapshot(report.parent_path()/L"toggle-middle.png");motion.began=GetTickCount64()-200;image=image&&snapshot(report.parent_path()/L"toggle-end.png");toggleTest=toggleTest&&motionValue(motion,GetTickCount64())==motion.target;}else toggleTest=motionValue(motion,GetTickCount64())==motion.target;toggleTest=toggleTest&&settings.value("warning",true)!=warning;activate(100);activate(200);
  std::ifstream saved(data/L"settings.json");auto stored=json::parse(saved);preferences=preferences&&stored["language"]=="it"&&stored["theme"]=="dark";
  interactionTest=typed&&protectedAction&&cancelled&&preferences&&toggleTest&&displayFamily&&boldFamily&&bodyFamily&&mediumFamily&&image;
  selected=doze::Action::Restart;countdown.start(selected,1,false,GetTickCount64());testPhase=interactionTest?1:-1;
  if(testPhase<0)finishTest(false,"input, action protection, preferences or rendering failure");
 }else if(testPhase==1&&!simulated.empty()){modal=Modal::None;finishTest(true,"");}}

 void finishTest(bool ok,const std::string& error){json result={{"ok",ok},{"nativeUI",true},{"renderer","Win32 + GDI+"},{"webview",false},{"error",error},{"fontFamiliesLoaded",displayFamily&&bodyFamily},{"keyboardAndControls",interactionTest},{"toggleTransitions",toggleTest},{"simulatedActions",simulated},{"firstFrameMs",firstPaint?firstPaint-started:0},{"scrollbars",false},{"width",width},{"height",height}};std::ofstream out(report,std::ios::binary);out<<result.dump(2);out.close();report.clear();PostMessageW(window,WM_APP+1,0,0);}
};
App* application=nullptr;
#include "accessibility.hpp"
DWORD powercfg(const std::wstring& options){wchar_t dir[MAX_PATH];GetSystemDirectoryW(dir,MAX_PATH);std::wstring cmd=L"\""+std::wstring(dir)+L"\\powercfg.exe\" "+options;STARTUPINFOW s{};s.cb=sizeof(s);PROCESS_INFORMATION p{};if(!CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&s,&p))return GetLastError();WaitForSingleObject(p.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(p.hProcess,&code);CloseHandle(p.hThread);CloseHandle(p.hProcess);return code;}
LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM w,LPARAM l){auto a=application;if(!a)return DefWindowProcW(hwnd,message,w,l);switch(message){case WM_GETOBJECT:if((LONG)l==OBJID_CLIENT){auto acc=new Accessible;auto result=LresultFromObject(IID_IAccessible,w,acc);acc->Release();return result;}break;case WM_ERASEBKGND:return 1;case WM_PAINT:a->paint();return 0;case WM_SIZE:a->invalidate();return 0;case WM_DPICHANGED:{auto r=(RECT*)l;SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);return 0;}case WM_MOUSEMOVE:{int id=a->at(GET_X_LPARAM(l)/a->dpi,GET_Y_LPARAM(l)/a->dpi);if(id!=a->hot){a->hot=id;a->invalidate();}TRACKMOUSEEVENT t{sizeof(t),TME_LEAVE,hwnd,0};TrackMouseEvent(&t);return 0;}case WM_MOUSELEAVE:a->hot=0;a->invalidate();return 0;case WM_SETCURSOR:if(LOWORD(l)==HTCLIENT){SetCursor(LoadCursorW(nullptr,a->hot>=60&&a->hot<=62?IDC_IBEAM:a->hot?IDC_HAND:IDC_ARROW));return TRUE;}break;case WM_LBUTTONDOWN:{SetFocus(hwnd);int id=a->at(GET_X_LPARAM(l)/a->dpi,GET_Y_LPARAM(l)/a->dpi);a->keyboard=false;if(id){a->focus=id;a->activate(id);}else if(GET_Y_LPARAM(l)/a->dpi<68){ReleaseCapture();SendMessageW(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,0);}return 0;}case WM_KEYDOWN:a->key(w);return 0;case WM_CHAR:a->character(w);return 0;case WM_CLOSE:a->close();return 0;case WM_TIMER:if(w==2)a->animate();else a->timer();return 0;case WM_APP+1:DestroyWindow(hwnd);return 0;case WM_POWERBROADCAST:if((w==PBT_APMRESUMEAUTOMATIC||w==PBT_APMRESUMESUSPEND)&&a->countdown.active){a->countdown.cancel();a->notify(a->tr(L"Countdown annullato dopo la riattivazione del PC.",L"Countdown cancelled after the PC resumed."));}return TRUE;case WM_DESTROY:a->countdown.cancel();KillTimer(hwnd,1);KillTimer(hwnd,2);if(a->helper){CloseHandle(a->helper);a->helper=nullptr;}PostQuitMessage(0);return 0;}return DefWindowProcW(hwnd,message,w,l);}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){ULONGLONG started=GetTickCount64();int argc;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path data,report;bool test=false;std::wstring config;for(int i=1;i<argc;i++){std::wstring arg=argv[i];if(arg==L"--test-mode")test=true;else if(arg.starts_with(L"--data-dir="))data=arg.substr(11);else if(arg.starts_with(L"--smoke-report="))report=arg.substr(15);else if(arg.starts_with(L"--configure-hibernation="))config=arg.substr(24);}LocalFree(argv);
 if(!config.empty()){if(test)return ERROR_ACCESS_DENIED;if(config==L"enable"){auto e=powercfg(L"/hibernate on");return e?e:powercfg(L"/hibernate /type full");}return config==L"disable"?powercfg(L"/hibernate off"):ERROR_INVALID_PARAMETER;}if(!report.empty()&&!test)return ERROR_ACCESS_DENIED;
 if(data.empty()){PWSTR path=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&path)))return 1;data=fs::path(path)/L"CustomInfo/Doze";CoTaskMemFree(path);}ULONG_PTR graphicsToken;GdiplusStartupInput input;if(GdiplusStartup(&graphicsToken,&input,nullptr)!=Ok)return 1;
 CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);int result=0;{App a;application=&a;a.started=started;a.instance=instance;a.data=data;a.report=report;a.test=test;BOOL animation=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&animation,0);a.motionAllowed=animation!=FALSE;try{a.load();a.loadFonts();}catch(const std::exception& e){MessageBoxW(nullptr,wide(e.what()).c_str(),L"Doze",MB_OK|MB_ICONERROR);application=nullptr;return 1;}
 std::wstring mutexName=test?L"Local\\CustomInfo.Doze.Test."+std::to_wstring(std::hash<std::wstring>{}(data.wstring())):L"Local\\CustomInfo.Doze";HANDLE mutex=CreateMutexW(nullptr,FALSE,mutexName.c_str());if(GetLastError()==ERROR_ALREADY_EXISTS){auto old=FindWindowW(L"CustomInfo.Doze",nullptr);if(old){ShowWindow(old,SW_RESTORE);SetForegroundWindow(old);}CloseHandle(mutex);application=nullptr;return 0;}
 SetCurrentProcessExplicitAppUserModelID(L"CustomInfo.Doze");UINT dpi=GetDpiForSystem();WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=procedure;wc.lpszClassName=L"CustomInfo.Doze";wc.hIcon=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(2),IMAGE_ICON,MulDiv(32,dpi,96),MulDiv(32,dpi,96),0);wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);int width=MulDiv(580,dpi,96),height=std::min(MulDiv(840,dpi,96),(int)(work.bottom-work.top-20));
 a.window=CreateWindowExW(WS_EX_APPWINDOW,L"CustomInfo.Doze",test?L"Doze · TEST":L"Doze",WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,width,height,nullptr,nullptr,instance,nullptr);if(!a.window)return 1;SendMessageW(a.window,WM_SETICON,ICON_SMALL,(LPARAM)LoadImageW(instance,MAKEINTRESOURCEW(2),IMAGE_ICON,MulDiv(16,dpi,96),MulDiv(16,dpi,96),0));a.prepareFrame();ShowWindow(a.window,show);InvalidateRect(a.window,nullptr,FALSE);UpdateWindow(a.window);SetTimer(a.window,1,100,nullptr);MSG m;while(GetMessageW(&m,nullptr,0,0)>0){TranslateMessage(&m);DispatchMessageW(&m);}CloseHandle(mutex);application=nullptr;}
 CoUninitialize();GdiplusShutdown(graphicsToken);return result;
}
