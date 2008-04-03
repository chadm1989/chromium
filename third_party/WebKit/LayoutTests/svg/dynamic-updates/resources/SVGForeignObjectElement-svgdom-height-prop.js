// [Name] SVGForeignObjectElement-svgdom-height-prop.js
// [Expected rendering result] 'Test passed' text - and a series of PASS mesages

description("Tests dynamic updates of the 'height' property of the SVGForeignObjectElement object")
createSVGTestCase();

var foreignObjectElement = createSVGElement("foreignObject");
foreignObjectElement.setAttribute("x", "100");
foreignObjectElement.setAttribute("y", "80");
foreignObjectElement.setAttribute("width", "150");
foreignObjectElement.setAttribute("height", "0");

var htmlDivElement = document.createElementNS(xhtmlNS, "xhtml:div");
htmlDivElement.setAttribute("style", "background-color: green; color: white; text-align: center");
htmlDivElement.textContent = "Test passed";

foreignObjectElement.appendChild(htmlDivElement);
rootSVGElement.appendChild(foreignObjectElement);

shouldBe("foreignObjectElement.height.baseVal.value", "0");

function executeTest() {
    foreignObjectElement.height.baseVal.value = "150";
    shouldBe("foreignObjectElement.height.baseVal.value", "150");

    waitForClickEvent(foreignObjectElement);
    triggerUpdate();
}

executeTest();
var successfullyParsed = true;
