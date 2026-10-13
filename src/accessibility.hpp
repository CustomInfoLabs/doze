#pragma once
#include <oleacc.h>
// Expose custom-drawn controls through the native MSAA accessibility API.
class Accessible final:public IAccessible {
 LONG refs=1;
 App* app()const{return application;}
 const Hit* child(VARIANT value)const{auto a=app();return a&&value.vt==VT_I4&&value.lVal>0&&value.lVal<=(LONG)a->hits.size()?&a->hits[value.lVal-1]:nullptr;}
 HRESULT string(const std::wstring& value,BSTR* out){if(!out)return E_POINTER;*out=SysAllocString(value.c_str());return *out?S_OK:E_OUTOFMEMORY;}
public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out) override {if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IDispatch||id==IID_IAccessible){*out=static_cast<IAccessible*>(this);AddRef();return S_OK;}return E_NOINTERFACE;}
 ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&refs);}
 ULONG STDMETHODCALLTYPE Release() override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* count) override{if(!count)return E_POINTER;*count=0;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT,LCID,ITypeInfo**) override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID,LPOLESTR*,UINT,LCID,DISPID*) override{return DISP_E_UNKNOWNNAME;}
 HRESULT STDMETHODCALLTYPE Invoke(DISPID id,REFIID,LCID,WORD,DISPPARAMS* p,VARIANT* result,EXCEPINFO*,UINT*) override{
  if(!result)return E_POINTER;VariantInit(result);VARIANT v;VariantInit(&v);v.vt=VT_I4;v.lVal=0;if(p&&p->cArgs)v=p->rgvarg[0];
  if(id==DISPID_ACC_NAME){result->vt=VT_BSTR;return get_accName(v,&result->bstrVal);}if(id==DISPID_ACC_VALUE){result->vt=VT_BSTR;return get_accValue(v,&result->bstrVal);}if(id==DISPID_ACC_ROLE)return get_accRole(v,result);if(id==DISPID_ACC_STATE)return get_accState(v,result);if(id==DISPID_ACC_FOCUS)return get_accFocus(result);if(id==DISPID_ACC_CHILDCOUNT){result->vt=VT_I4;return get_accChildCount(&result->lVal);}if(id==DISPID_ACC_DEFAULTACTION){result->vt=VT_BSTR;return get_accDefaultAction(v,&result->bstrVal);}return DISP_E_MEMBERNOTFOUND;
 }
 HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** out) override{if(!out)return E_POINTER;*out=nullptr;auto a=app();return a?CreateStdAccessibleObject(a->window,OBJID_WINDOW,IID_IDispatch,(void**)out):E_FAIL;}
 HRESULT STDMETHODCALLTYPE get_accChildCount(LONG* n) override{if(!n)return E_POINTER;auto a=app();*n=a?(LONG)a->hits.size():0;return S_OK;}
 HRESULT STDMETHODCALLTYPE get_accChild(VARIANT,IDispatch** out) override{if(!out)return E_POINTER;*out=nullptr;return S_FALSE;}
 HRESULT STDMETHODCALLTYPE get_accName(VARIANT v,BSTR* out) override{auto h=child(v);return string(h?h->label:L"Doze",out);}
 HRESULT STDMETHODCALLTYPE get_accValue(VARIANT v,BSTR* out) override{auto a=app();auto h=child(v);if(!out)return E_POINTER;*out=nullptr;if(!a||!h||h->id<60||h->id>62)return S_FALSE;return string(std::to_wstring(h->id==60?a->hours:h->id==61?a->minutes:a->seconds),out);}
 HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT v,BSTR* out) override{auto h=child(v);return string(h?h->label:L"",out);}
 HRESULT STDMETHODCALLTYPE get_accRole(VARIANT v,VARIANT* out) override{if(!out)return E_POINTER;VariantInit(out);out->vt=VT_I4;auto h=child(v);out->lVal=!h?ROLE_SYSTEM_CLIENT:h->id>=60&&h->id<=62?ROLE_SYSTEM_TEXT:h->id==92||h->id==100||h->id==101?ROLE_SYSTEM_CHECKBUTTON:(h->id>=10&&h->id<=13)||(h->id>=102&&h->id<=104)?ROLE_SYSTEM_RADIOBUTTON:ROLE_SYSTEM_PUSHBUTTON;return S_OK;}
 HRESULT STDMETHODCALLTYPE get_accState(VARIANT v,VARIANT* out) override{if(!out)return E_POINTER;VariantInit(out);out->vt=VT_I4;out->lVal=STATE_SYSTEM_FOCUSABLE;auto a=app();auto h=child(v);if(a&&h){if(!h->enabled)out->lVal|=STATE_SYSTEM_UNAVAILABLE;if(a->focus==h->id)out->lVal|=STATE_SYSTEM_FOCUSED;bool checked=h->id==92?a->force:h->id==100?a->settings.value("warning",true):h->id==101?a->light():h->id>=10&&h->id<=13?(int)a->selected==h->id-10:h->id>=102&&h->id<=104?a->settings.value("language","auto")==std::string(h->id==102?"auto":h->id==103?"it":"en"):false;if(checked)out->lVal|=STATE_SYSTEM_CHECKED;}return S_OK;}
 HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT,BSTR* out) override{if(out)*out=nullptr;return S_FALSE;}
 HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* file,VARIANT,LONG* topic) override{if(file)*file=nullptr;if(topic)*topic=-1;return S_FALSE;}
 HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT,BSTR* out) override{if(out)*out=nullptr;return S_FALSE;}
 HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* out) override{if(!out)return E_POINTER;VariantInit(out);out->vt=VT_I4;out->lVal=0;auto a=app();if(a)for(size_t i=0;i<a->hits.size();i++)if(a->hits[i].id==a->focus){out->lVal=(LONG)i+1;break;}return S_OK;}
 HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* out) override{return get_accFocus(out);}
 HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT v,BSTR* out) override{auto a=app();auto h=child(v);if(!a||!h){if(out)*out=nullptr;return S_FALSE;}return string(h->id>=60&&h->id<=62?a->tr(L"Modifica",L"Edit"):a->tr(L"Premi",L"Press"),out);}
 HRESULT STDMETHODCALLTYPE accSelect(LONG flags,VARIANT v) override{auto a=app();auto h=child(v);if(!a||!h||!h->enabled)return E_INVALIDARG;if(flags&SELFLAG_TAKEFOCUS){a->focus=h->id;a->keyboard=true;SetFocus(a->window);a->invalidate();NotifyWinEvent(EVENT_OBJECT_FOCUS,a->window,OBJID_CLIENT,v.lVal);}return S_OK;}
 HRESULT STDMETHODCALLTYPE accLocation(LONG* x,LONG* y,LONG* w,LONG* h,VARIANT v) override{auto a=app();if(!a||!x||!y||!w||!h)return E_POINTER;auto c=child(v);POINT origin{};ClientToScreen(a->window,&origin);RectF r=c?c->box:RectF(0,0,a->width,a->height);*x=origin.x+(LONG)(r.X*a->dpi);*y=origin.y+(LONG)(r.Y*a->dpi);*w=(LONG)(r.Width*a->dpi);*h=(LONG)(r.Height*a->dpi);return S_OK;}
 HRESULT STDMETHODCALLTYPE accNavigate(LONG direction,VARIANT start,VARIANT* out) override{if(!out)return E_POINTER;VariantInit(out);auto a=app();if(!a)return E_FAIL;LONG n=start.vt==VT_I4?start.lVal:0;if(direction==NAVDIR_FIRSTCHILD)n=1;else if(direction==NAVDIR_LASTCHILD)n=(LONG)a->hits.size();else if(direction==NAVDIR_NEXT)n++;else if(direction==NAVDIR_PREVIOUS)n--;else return S_FALSE;if(n<=0||n>(LONG)a->hits.size())return S_FALSE;out->vt=VT_I4;out->lVal=n;return S_OK;}
 HRESULT STDMETHODCALLTYPE accHitTest(LONG x,LONG y,VARIANT* out) override{if(!out)return E_POINTER;VariantInit(out);auto a=app();if(!a)return E_FAIL;POINT p{x,y};ScreenToClient(a->window,&p);out->vt=VT_I4;out->lVal=0;for(size_t i=0;i<a->hits.size();i++)if(a->hits[i].box.Contains(p.x/a->dpi,p.y/a->dpi)){out->lVal=(LONG)i+1;break;}return S_OK;}
 HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT v) override{auto a=app();auto h=child(v);if(!a||!h||!h->enabled)return E_INVALIDARG;int id=h->id;a->activate(id);return S_OK;}
 HRESULT STDMETHODCALLTYPE put_accName(VARIANT,BSTR) override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE put_accValue(VARIANT v,BSTR value) override{auto a=app();auto h=child(v);if(!a||!h||!h->enabled||h->id<60||h->id>62||!value)return E_INVALIDARG;std::wstring text=value;if(text.empty()||text.size()>2||text.find_first_not_of(L"0123456789")!=std::wstring::npos)return E_INVALIDARG;int n=std::stoi(text);if(n>(h->id==60?99:59))return E_INVALIDARG;int& field=h->id==60?a->hours:h->id==61?a->minutes:a->seconds;field=n;a->invalidate();return S_OK;}
};
