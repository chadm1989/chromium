// [Name] SVGClipPathElement-dom-clipPathUnits-attr.js
// [Expected rendering result] green circle - and a series of PASS mesages

description("Tests dynamic updates of the 'clipPathUnits' attribute of the SVGClipPathElement object")
createSVGTestCase();

var defsElement = createSVGElement("defs");
rootSVGElement.appendChild(defsElement);

var clipPathElement = createSVGElement("clipPath");
clipPathElement.setAttribute("id", "clipper");
clipPathElement.setAttribute("clipPathUnits", "objectBoundingBox");

var circleElement = createSVGElement("circle");
circleElement.setAttribute("cx", "150");
circleElement.setAttribute("cy", "150");
circleElement.setAttribute("r", "150");
clipPathElement.appendChild(circleElement);

defsElement.appendChild(clipPathElement);;

var rectElement = createSVGElement("rect");
rectElement.setAttribute("width", "300");
rectElement.setAttribute("height", "300");
rectElement.setAttribute("fill", "green");
rectElement.setAttribute("clip-path", "url(#clipper)");
rootSVGElement.appendChild(rectElement);

shouldBeEqualToString("clipPathElement.getAttribute('clipPathUnits')", "objectBoundingBox")

function executeTest() {
    clipPathElement.setAttribute("clipPathUnits", "userSpaceOnUse");
    shouldBeEqualToString("clipPathElement.getAttribute('clipPathUnits')", "userSpaceOnUse");

    waitForClickEvent(rectElement);
    triggerUpdate();
}

window.setTimeout("executeTest()", 0);
var successfullyParsed = true;
