(async()=>{
 const wait=async fn=>{for(let i=0;i<120;i++){if(fn())return;await new Promise(r=>setTimeout(r,100));}throw new Error('UI timeout');};
 try{
  await wait(()=>window.DozeReady);const call=window.DozeNative.call;
  const initial=await call('state');if(!initial.testMode)throw new Error('Unsafe test');
  await call('settings',{language:'en',theme:'light',warning:true});await wait(()=>document.documentElement.lang==='en'&&document.documentElement.classList.contains('light'));
  const start=document.getElementById('start');const primary=getComputedStyle(start);
  const whiteText=primary.color==='rgb(255, 255, 255)'&&primary.fontSize==='12px'&&primary.fontWeight==='700';
  document.getElementById('about-open').click();const about=document.getElementById('about-dialog');await wait(()=>about.open);const aboutTranslated=about.querySelector('dt').textContent==='Version';about.close();
  await call('start',{action:'shutdown',seconds:30,force:false});await wait(()=>!document.getElementById('cancel').disabled);await call('cancel');await wait(()=>document.getElementById('cancel').disabled);
  let rejected=false;try{await call('start',{action:'invalid',seconds:1,force:false});}catch{rejected=true;}
  await call('start',{action:'restart',seconds:1,force:false});await wait(()=>document.getElementById('info-dialog').open);document.getElementById('info-dialog').close();
  await call('settings',{language:'it',theme:'dark',warning:false});await wait(()=>document.documentElement.lang==='it');
  await document.fonts.ready;
  const main=document.querySelector('main'),bar=document.querySelector('.titlebar');
  const overflow=document.documentElement.scrollWidth>document.documentElement.clientWidth;
  const verticalOverflow=main.scrollHeight>main.clientHeight;
  const top=bar.getBoundingClientRect().top;
  main.style.height='260px';main.scrollTop=200;
  const fixedTopbar=bar.getBoundingClientRect().top===top&&document.documentElement.scrollTop===0;
  main.style.height='';main.scrollTop=0;
  const layout={viewportHeight:innerHeight,contentHeight:main.scrollHeight,availableHeight:main.clientHeight};
  const report={ok:whiteText&&aboutTranslated&&rejected&&!overflow&&!verticalOverflow&&fixedTopbar,whiteText,aboutTranslated,rejected,overflow,verticalOverflow,fixedTopbar,layout,startup:initial.startup,capabilities:initial.capabilities};
  await call('test-result',report);
 }catch(error){await window.DozeNative.call('test-result',{ok:false,error:error.message});}
})()
