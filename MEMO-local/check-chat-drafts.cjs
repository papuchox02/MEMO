const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));await page.locator('.splash').click();
const chat=async()=>page.locator('[data-go="chat"]').click(),back=async()=>page.locator('[data-action="back"]').click();await chat();
assert.equal(await page.locator('.chat-layout aside .eye').count(),0);
for(let i=0;i<4;i++)await page.locator('[data-action="new-chat"]').click();assert.equal(await page.locator('.chat-option').count(),1);
await back();await chat();assert.equal(await page.locator('.chat-option').count(),0);
await page.locator('[data-action="new-chat"]').click();await page.locator('[name="message"]').fill('Borrador pendiente');await back();await chat();assert.equal(await page.locator('[name="message"]').inputValue(),'Borrador pendiente');
await page.locator('[data-action="new-chat"]').click();assert.equal(await page.locator('.chat-option').count(),1);assert.equal(await page.locator('[name="message"]').inputValue(),'Borrador pendiente');
await page.reload();await page.locator('.splash').click();await chat();await page.locator('[data-action="new-chat"]').click();assert.equal(await page.locator('[name="message"]').inputValue(),'Borrador pendiente');
await page.getByRole('button',{name:'Enviar',exact:true}).click();assert.equal(await page.locator('[name="message"]').inputValue(),'');
await page.locator('[data-action="new-chat"]').click();assert.equal(await page.locator('.chat-option').count(),2);
await page.locator('.chat-choice').filter({hasText:'Borrador pendiente'}).click();assert.equal(await page.locator('.chat-option').count(),1);
await page.locator('[data-action="new-chat"]').click();await page.locator('[name="message"]').fill('Temporal');await page.locator('[name="message"]').fill('');await back();await chat();assert.equal(await page.locator('.chat-option').count(),1);
assert.deepEqual(errors,[]);console.log('OK: sin ojos laterales, nuevo chat sin duplicados, limpieza al salir/cambiar, borradores persistentes y campo limpio al enviar.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
