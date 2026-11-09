const {chromium}=require('C:/Users/ADMIN/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/playwright');
const assert=require('node:assert/strict'),path=require('node:path');
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'});try{
const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));await page.goto('file:///'+path.resolve(__dirname,'MEMO.html').replace(/\\/g,'/'));
const route=async(from,key,to)=>{await page.locator(from).focus();await page.keyboard.press(key);assert.equal(await page.locator('#screen .joystick-selected').evaluate((e,s)=>e.matches(s),to),true,from+' '+key+' → '+to);};
await page.keyboard.press('ArrowDown');await page.keyboard.press('Enter');
await route('[data-go="calendar"]','ArrowLeft','footer [data-action="settings"]');
await route('footer [data-action="settings"]','ArrowRight','#network');
await route('#network','ArrowLeft','footer [data-action="settings"]');
await route('footer [data-action="settings"]','ArrowUp','[data-go="calendar"]');
await page.locator('[data-go="pomodoro"]').focus();await page.keyboard.press('Enter');await page.waitForTimeout(40);assert.equal(await page.locator('.joystick-selected').getAttribute('data-preset'),'15,2');
for(const section of ['pomodoro','chat','calendar']){
 if(section!=='pomodoro'){await page.locator('header [data-action="back"]').click();await page.locator('[data-go="'+section+'"]').click();}
 await route('footer [data-action="settings"]','ArrowRight','#network');await route('#network','ArrowLeft','footer [data-action="settings"]');
}
for(const [key,to]of [['ArrowUp','header [data-action="back"]'],['ArrowLeft','[data-action="new-task"]'],['ArrowRight','[data-mode="year"]'],['ArrowDown','[data-month="-1"]']])await route('[data-action="calendar-expand"]',key,to);
await page.locator('[data-action="calendar-expand"]').click();await route('[data-action="calendar-expand"]','ArrowUp','header [data-action="back"]');await route('[data-action="calendar-expand"]','ArrowDown','[data-month="-1"]');
assert.deepEqual(errors,[]);console.log('OK: rutas del menú, usuario/Wi-Fi en todas las pantallas, primera sesión de Pomodoro y cuatro direcciones desde ampliar Calendario.');
}finally{await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
