const canvas = document.getElementById('lcd');
const ctx = canvas.getContext('2d');

const pixelSize = 6;
const pixelGap = 2;

const charWidth = 5;
const charHeight = 8;

const columns = 16;
const rows = 2;

const charPixelWidth = charWidth * (pixelSize + pixelGap);
const charPixelHeight = charHeight * (pixelSize + pixelGap);

canvas.width = columns * charPixelWidth;
canvas.height = rows * charPixelHeight;

function drawPixel(x, y) {
    ctx.fillStyle = '#00FF88';
    ctx.fillRect(
        x * (pixelSize + pixelGap),
        y * (pixelSize + pixelGap),
        pixelSize,
        pixelSize
    );
}

function drawCharacter(startX, startY) {
    for (let y = 0; y < charHeight; y++) {
        for (let x = 0; x < charWidth; x++) {
            drawPixel(startX + x, startY + y);
        }
    }
}

function drawLCD() {
    for (let row = 0; row < rows; row++) {
        for (let column = 0; column < columns; column++) {
            
            const offsetX = column * charWidth;
            const offsetY = row * charHeight;

            drawCharacter(offsetX, offsetY);
        }
    }
}

drawLCD();