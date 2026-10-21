const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage({viewport:{width:1200,height:900}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
const open=async()=>{await page.locator('.splash').click();await page.locator('[data-go="pomodoro"]').click();};
await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));await open();
await page.locator('[data-remove-preset="15,2"]').click();await page.locator('#modal').getByRole('button',{name:'Volver',exact:true}).click();assert.equal(await page.locator('[data-preset]').count(),4);
await page.locator('#timer-toggle').click();await page.locator('[data-remove-preset="25,5"]').click();await page.locator('#confirm-action').click();
assert.equal(await page.locator('[data-preset="25,5"]').count(),0);assert.equal(await page.locator('#timer-toggle').textContent(),'Pausar');
await page.locator('#timer-toggle').click();await page.reload();await open();assert.equal(await page.locator('[data-preset="25,5"]').count(),0);
for(const pair of ['15,2','50,10','90,30']){await page.locator('[data-remove-preset="'+pair+'"]').click();await page.locator('#confirm-action').click();}
assert.equal(await page.locator('[data-preset]').count(),0);await page.reload();await open();assert.equal(await page.locator('[data-preset]').count(),0);
await page.locator('[data-action="custom-timer"]').click();await page.locator('[name="focus"]').fill('25');await page.locator('[name="rest"]').fill('5');await page.getByRole('button',{name:'Guardar sesión'}).click();if(await page.locator('#confirm-action').isVisible())await page.locator('#confirm-action').click();assert.equal(await page.locator('[data-preset="25,5"]').count(),1);
await page.locator('[data-remove-preset="25,5"]').click();await page.locator('#confirm-action').click();assert.equal(await page.locator('[data-preset]').count(),0);assert.deepEqual(errors,[]);
console.log('OK: confirmar/cancelar eliminación, borrar predeterminadas y personalizadas, persistencia de lista vacía y sesión activa sin interrupciones.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
