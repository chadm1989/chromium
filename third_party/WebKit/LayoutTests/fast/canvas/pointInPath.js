description("Series of tests for Canvas.isPointInPath");

ctx = document.getElementById("canvas").getContext("2d");
ctx.save();
debug("Rectangle at (0,0) 20x20");
ctx.rect(0, 0, 20, 20);
shouldBe("ctx.isPointInPath(5, 5)", "true");
shouldBe("ctx.isPointInPath(10, 10)", "true");
shouldBe("ctx.isPointInPath(19, 19)", "true");
shouldBe("ctx.isPointInPath(30, 30)", "false");
shouldBe("ctx.isPointInPath(-1, 10)", "false");
shouldBe("ctx.isPointInPath(10, -1)", "false");
debug("Translate context (10,10)");
ctx.translate(10,10);
shouldBe("ctx.isPointInPath(5, 5)", "true");
shouldBe("ctx.isPointInPath(10, 10)", "true");
shouldBe("ctx.isPointInPath(19, 19)", "true");
shouldBe("ctx.isPointInPath(30, 30)", "false");
shouldBe("ctx.isPointInPath(-1, 10)", "false");
shouldBe("ctx.isPointInPath(10, -1)", "false");
debug("Collapse ctm to non-invertible matrix");
ctx.scale(0,0);
shouldBe("ctx.isPointInPath(5, 5)", "false");
shouldBe("ctx.isPointInPath(10, 10)", "false");
shouldBe("ctx.isPointInPath(20, 20)", "false");
shouldBe("ctx.isPointInPath(30, 30)", "false");
shouldBe("ctx.isPointInPath(-1, 10)", "false");
shouldBe("ctx.isPointInPath(10, -1)", "false");
debug("Resetting context to a clean state");
ctx.restore();
ctx.beginPath();
debug("Translate context (10,10)");
ctx.translate(10,10);
debug("Rectangle at (0,0) 20x20");
ctx.rect(0, 0, 20, 20);
shouldBe("ctx.isPointInPath(5, 5)", "false");
shouldBe("ctx.isPointInPath(10, 10)", "true");
shouldBe("ctx.isPointInPath(20, 20)", "true");
shouldBe("ctx.isPointInPath(29, 29)", "true");
shouldBe("ctx.isPointInPath(-1, 10)", "false");
shouldBe("ctx.isPointInPath(10, -1)", "false");

var successfullyParsed = true;
