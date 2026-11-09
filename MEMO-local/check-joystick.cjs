const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage({viewport:{width:1200,height:950}}),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));
const move=async dir=>{await page.locator('.joystick [data-dir="'+dir+'"]').click();await page.waitForTimeout(30);};const select=async()=>{await page.locator('.joystick [data-action="select"]').click();await page.waitForTimeout(80);};
const target=()=>page.locator('#screen .joystick-selected');
const up=await page.locator('.joystick [data-dir="up"]').boundingBox(),down=await page.locator('.joystick [data-dir="down"]').boundingBox(),left=await page.locator('.joystick [data-dir="left"]').boundingBox(),right=await page.locator('.joystick [data-dir="right"]').boundingBox(),center=await page.locator('.joystick [data-action="select"]').boundingBox();assert.ok(up.y<center.y&&down.y>center.y&&left.x<center.x&&right.x>center.x);assert.equal(up.x,down.x);assert.equal(left.y,right.y);
await select();assert.equal(await page.locator('.home').count(),1);assert.equal(await target().getAttribute('data-go'),'pomodoro');
await move('down');assert.equal(await target().getAttribute('data-go'),'chat');await move('up');assert.equal(await target().getAttribute('data-go'),'pomodoro');
await select();assert.equal(await page.locator('.pomodoro-layout').count(),1);assert.equal(await target().getAttribute('data-action'),'edit-sessions');
await move('down');assert.equal(await target().getAttribute('data-preset'),'15,2');await select();assert.equal(await page.locator('#remaining').textContent(),'15:00');assert.equal(await target().getAttribute('data-preset'),'15,2');
await move('up');await select();assert.equal(await target().getAttribute('data-action'),'edit-sessions');await move('right');assert.equal(await target().getAttribute('data-action'),'custom-timer');await select();assert.equal(await page.locator('#modal').isVisible(),true);assert.equal(await target().getAttribute('name'),'focus');
const old=Number(await target().inputValue());await select();await move('up');assert.equal(Number(await target().inputValue()),old+1);await select();await move('right');assert.equal(await target().getAttribute('name'),'rest');await page.keyboard.press('Escape');assert.equal(await page.locator('#modal').isVisible(),false);
// Header and footer must be reachable from the joystick, not just content.
let foundBack=false;for(let i=0;i<8;i++){await move('up');if(await target().getAttribute('data-action')==='back'){foundBack=true;break;}}assert.equal(foundBack,true);await select();assert.equal(await page.locator('.home').count(),1);
await move('left');await move('down');let footer=false;for(let i=0;i<7;i++){if(await target().evaluate(e=>!!e.closest('footer'))){footer=true;break;}await move('down');}assert.equal(footer,true);
// Touch/click remains usable; joystick continues from the clicked control.
await page.locator('#view [data-go="calendar"]').click();assert.equal(await page.locator('.calendar-layout').count(),1);await page.locator('[data-mode="month"]').click();await move('down');assert.equal(await target().count(),1);
await page.locator('[data-action="settings"]').click();await move('down');assert.equal(await target().evaluate(e=>!!e.closest('#modal')),true);assert.equal(await page.locator('#screen .joystick-selected').count(),1);
await page.screenshot({path:path.join(__dirname,'MEMO-joystick.png')});assert.deepEqual(errors,[]);console.log('OK: cruz, selección naranja, navegación direccional, botón central, continuidad tras redibujar, formularios, encabezado/pie, ventanas y clic táctil.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
