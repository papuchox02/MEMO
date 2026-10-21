const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict');
const path=require('node:path');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});
 try{
 const page=await browser.newPage({viewport:{width:1200,height:900},reducedMotion:'reduce'});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));
 await page.locator('.splash').click();
 assert.equal(await page.locator('.welcome').evaluate(e=>e.scrollWidth<=e.clientWidth),true,'El saludo cabe en una línea');
 const face=await page.locator('.home-face').boundingBox(),eyes=await page.locator('.home-face .eyes').boundingBox();
 assert.ok(Math.abs(face.x+face.width/2-eyes.x-eyes.width/2)<1);
 assert.ok(Math.abs(face.y+face.height/2-eyes.y-eyes.height/2)<1);
 assert.equal(await page.locator('.home-menu svg').count(),3);
 const statusBefore=await page.locator('#status').boundingBox();
 await page.locator('#network').click();
 const statusAfter=await page.locator('#status').boundingBox();
 assert.equal(statusBefore.x,statusAfter.x,'El nombre del modo no se mueve al desconectar');
 const screen=await page.locator('#screen').boundingBox();
 assert.ok(Math.abs(statusAfter.x+statusAfter.width/2-screen.x-screen.width/2)<1,'Nombre del modo centrado');
 await page.locator('#network').click();
 assert.equal(await page.locator('#network').textContent(),'Wi-Fi');
 assert.equal(await page.locator('footer .user-icon').count(),1);
 const greeting=await page.locator('.welcome').boundingBox();
 assert.ok(greeting.y-(eyes.y+eyes.height)<25,'Saludo cerca de los ojos');
 await page.locator('[data-action="settings"]').click();
 assert.equal(await page.locator('#modal .user-icon').count(),1);
 await page.locator('[data-action="close-modal"]').click();
 await page.waitForTimeout(3600);
 await page.screenshot({path:path.join(__dirname,'MEMO-menu.png')});
 await page.emulateMedia({reducedMotion:'no-preference'});
 assert.equal(await page.locator('.home-face .eyes').evaluate(e=>getComputedStyle(e).animationName),'playful-look');
 const startTransform=await page.locator('.home-face .eyes').evaluate(e=>getComputedStyle(e).transform);
 await page.waitForTimeout(2400);
 assert.notEqual(await page.locator('.home-face .eyes').evaluate(e=>getComputedStyle(e).transform),startTransform,'La mirada se desplaza');
 for(const mode of ['pomodoro','chat','calendar']){await page.locator('[data-go="'+mode+'"]').click();await page.locator('[data-action="back"]').click();}
 await page.locator('[data-action="back"]').click();assert.equal(await page.locator('.splash').count(),1);
 assert.deepEqual(errors,[]);
 console.log('OK: saludo cercano, mirada animada, icono de usuario y pie centrado con y sin conexión; navegación correcta.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
