const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage({viewport:{width:1200,height:900},reducedMotion:'reduce'}),errors=[];page.on('pageerror',e=>errors.push(e.message));
await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));await page.locator('.splash').click();await page.locator('[data-go="chat"]').click();
assert.equal(await page.locator('.chat-layout aside .eye').count(),2);assert.equal(await page.locator('[data-action="new-chat"]').evaluate(e=>e.scrollWidth<=e.clientWidth),true);
await page.locator('[data-action="new-chat"]').click();await page.locator('[name="message"]').fill('Mis tareas');await page.getByRole('button',{name:'Enviar',exact:true}).click();
await page.locator('[data-action="new-chat"]').click();await page.locator('[name="message"]').fill('Mis eventos');await page.getByRole('button',{name:'Enviar',exact:true}).click();
await page.getByRole('button',{name:'Editar conversaciones',exact:true}).click();await page.mouse.move(0,0);assert.equal(await page.locator('[data-remove-chat]').last().isVisible(),false);
const row=page.locator('.chat-option').last();await row.locator('[data-chat]').hover();assert.equal(await row.locator('[data-remove-chat]').isVisible(),true);
await row.locator('[data-remove-chat]').click();await page.locator('#modal [data-action="close-modal"]').first().click();assert.equal(await page.locator('.chat-option').count(),2);
await row.locator('[data-chat]').hover();await row.locator('[data-remove-chat]').click();await page.locator('#confirm-action').click();assert.equal(await page.locator('.chat-option').count(),1);
await page.reload();await page.locator('.splash').click();await page.locator('[data-go="chat"]').click();assert.equal(await page.locator('.chat-option').count(),1);
await page.screenshot({path:path.join(__dirname,'MEMO-chatbot.png')});assert.deepEqual(errors,[]);console.log('OK: ojos y barra compacta, nuevo chat en una línea, edición contextual, confirmación y borrado persistente.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});

