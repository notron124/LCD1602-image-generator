import { LCD1602 } from "./lcd1602.js";

const lcd = new LCD1602();

lcd.setCursor(17, 0);
console.log(lcd.cursor);

lcd.write('HELLO');
lcd.print();
lcd.clear();
lcd.print();
console.log(lcd.cursor);


