// This is a regression test for bug https://bugs.webkit.org/show_bug.cgi?id=89018

description("Tests that disabling the imageSmoothingEnabled attribute still works after multiple repaints");
var dstCanvas = document.getElementById("destination");
var dstCtx = dstCanvas.getContext('2d');
var srcCanvas = document.getElementById("source");
var srcCtx = srcCanvas.getContext('2d');


var srcCanvas, srcCtx, dstCanvas, dstCtx;

function draw()
{
    srcCtx.clearRect(0, 0, 300, 300);
    dstCtx.clearRect(0, 0, 300, 300);
    srcCtx.fillStyle = "rgb(255, 0, 0)";
    srcCtx.fillRect(0, 0, 1, 1);
    srcCtx.fillStyle = "rgb(0, 255, 0)";
    srcCtx.fillRect(1, 0, 1, 1);
    dstCtx.imageSmoothingEnabled = false;
    dstCtx.drawImage(srcCanvas, 0, 0, 2, 1, 0, 0, 300, 300);
}

function testResult() {
    debug("Test that the image is not filtered");
    left_of_center_pixel = dstCtx.getImageData(149, 150, 1, 1);
    shouldBe("left_of_center_pixel.data[0]", "255");
    shouldBe("left_of_center_pixel.data[1]", "0");
    shouldBe("left_of_center_pixel.data[2]", "0");
    right_of_center_pixel = dstCtx.getImageData(150, 150, 1, 1);
    shouldBe("right_of_center_pixel.data[0]", "0");
    shouldBe("right_of_center_pixel.data[1]", "255");
    shouldBe("right_of_center_pixel.data[2]", "0");
    finishJSTest();
}

// Bug 89018 requires 2 draw iteration in order to manifest itself.
var drawIterations = 2;

function BrowserPaint(){
    draw();
    if (drawIterations > 0) {
        drawIterations = drawIterations - 1;
        window.requestAnimationFrame(BrowserPaint);
    } else {
        testResult();
    }
}

function onLoadHandler()
{
    BrowserPaint();
}

window.jsTestIsAsync = true;
window.onload = onLoadHandler;

