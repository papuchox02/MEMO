const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage({viewport:{width:1200,height:900}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
const open=async()=>{await page.locator('.splash').click();await page.locator('[data-go="pomodoro"]').click();};
await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));await open();
assert.equal(await page.locator('[data-action="custom-timer"]').isVisible(),false);assert.equal(await page.locator('[data-remove-preset]').first().isVisible(),false);
const heading=await page.locator('.session-heading').boundingBox(),sidebar=await page.locator('#view aside').boundingBox();assert.ok(heading.width<sidebar.width);
await page.locator('#timer-toggle').click();await page.getByRole('button',{name:'Editar sesiones',exact:true}).click();await page.getByRole('button',{name:'Agregar sesión',exact:true}).click();
await page.locator('[name="focus"]').fill('37');await page.locator('[name="rest"]').fill('7');await page.getByRole('button',{name:'Guardar sesión',exact:true}).click();
assert.equal(await page.locator('[data-preset="37,7"]').count(),1);assert.equal(await page.locator('#timer-toggle').textContent(),'Pausar');
await page.mouse.move(0,0);assert.equal(await page.locator('[data-remove-preset="15,2"]').isVisible(),false);await page.locator('[data-preset="15,2"]').hover();assert.equal(await page.locator('[data-remove-preset="15,2"]').isVisible(),true);await page.locator('[data-remove-preset="15,2"]').click();await page.locator('#confirm-action').click();assert.equal(await page.locator('[data-preset="15,2"]').count(),0);
await page.locator('#timer-toggle').click();await page.reload();await open();
assert.equal(await page.locator('[data-preset="37,7"]').count(),1);assert.equal(await page.locator('[data-preset="15,2"]').count(),0);
await page.screenshot({path:path.join(__dirname,'MEMO-pomodoro.png')});await page.getByRole('button',{name:'Editar sesiones',exact:true}).click();await page.screenshot({path:path.join(__dirname,'MEMO-editor-sesiones.png')});
assert.deepEqual(errors,[]);console.log('OK: lápiz alterna edición, + abre el formulario, papelera contextual, altas/bajas persistentes y temporizador continuo.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
