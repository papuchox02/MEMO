const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage({viewport:{width:1200,height:900}}),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));await page.locator('.splash').click();await page.locator('[data-go="chat"]').click();
assert.equal(await page.locator('.chat input').count(),0);assert.equal(await page.locator('[data-action="send-chat"]').isDisabled(),true);
const voice=page.locator('[data-action="voice"]'),send=page.locator('[data-action="send-chat"]');const a=await voice.boundingBox(),b=await send.boundingBox();assert.ok(a.x<b.x);assert.ok(a.width>=120&&b.width>=120);
await voice.click();await page.locator('#draft-preview').waitFor();const partial=await page.locator('#draft-preview>span').textContent();assert.ok(partial.length>0&&partial.length<'¿Qué tareas tengo para hoy?'.length);assert.equal(await page.locator('.memo-reply').count(),0);
await page.waitForFunction(()=>document.querySelector('[data-action="voice"]').getAttribute('aria-pressed')==='false');assert.equal(await page.locator('#draft-preview>span').textContent(),'¿Qué tareas tengo para hoy?');assert.equal(await send.isEnabled(),true);
await page.screenshot({path:path.join(__dirname,'MEMO-transcripcion.png')});await send.click();assert.equal(await page.locator('#draft-preview').count(),0);assert.equal(await page.locator('.memo-reply').count(),1);
await voice.click();await page.locator('#draft-preview').waitFor();await page.locator('[data-action="back"]').click();await page.waitForTimeout(700);await page.locator('[data-go="chat"]').click();assert.equal(await voice.getAttribute('aria-pressed'),'false');assert.equal(await page.locator('#draft-preview').count(),1);
await page.reload();await page.locator('.splash').click();await page.locator('[data-go="chat"]').click();await page.locator('.chat-choice').click();assert.equal(await page.locator('#draft-preview').count(),1);
assert.deepEqual(errors,[]);console.log('OK: botones centrados sin campo de texto, transcripción gradual, envío explícito, respuesta con avatar y borrador conservado al salir.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
