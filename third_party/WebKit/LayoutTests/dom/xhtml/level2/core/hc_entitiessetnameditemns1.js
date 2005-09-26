
/*
Copyright Â© 2001-2004 World Wide Web Consortium, 
(Massachusetts Institute of Technology, European Research Consortium 
for Informatics and Mathematics, Keio University). All 
Rights Reserved. This work is distributed under the W3CÂ® Software License [1] in the 
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

[1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
*/



   /**
    *  Gets URI that identifies the test.
    *  @return uri identifier of test
    */
function getTargetURI() {
      return "http://www.w3.org/2001/DOM-Test-Suite/level2/core/hc_entitiessetnameditemns1";
   }

var docsLoaded = -1000000;
var builder = null;

//
//   This function is called by the testing framework before
//      running the test suite.
//
//   If there are no configuration exceptions, asynchronous
//        document loading is started.  Otherwise, the status
//        is set to complete and the exception is immediately
//        raised when entering the body of the test.
//
function setUpPage() {
   setUpPageStatus = 'running';
   try {
     //
     //   creates test document builder, may throw exception
     //
     builder = createConfiguredBuilder();

      docsLoaded = 0;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      docsLoaded += preload(docRef, "doc", "hc_staff");
        
       if (docsLoaded == 1) {
          setUpPage = 'complete';
       }
    } catch(ex) {
    	catchInitializationError(builder, ex);
        setUpPage = 'complete';
    }
}



//
//   This method is called on the completion of 
//      each asychronous load started in setUpTests.
//
//   When every synchronous loaded document has completed,
//      the page status is changed which allows the
//      body of the test to be executed.
function loadComplete() {
    if (++docsLoaded == 1) {
        setUpPageStatus = 'complete';
    }
}


/**
* 
An attempt to add an element to the named node map returned by entities should 
result in a NO_MODIFICATION_ERR or HIERARCHY_REQUEST_ERR.

* @author Curt Arnold
* @see http://www.w3.org/TR/DOM-Level-2-Core/core#ID-1788794630
* @see http://www.w3.org/TR/DOM-Level-2-Core/core#ID-setNamedItemNS
*/
function hc_entitiessetnameditemns1() {
   var success;
    if(checkInitialization(builder, "hc_entitiessetnameditemns1") != null) return;
    var doc;
      var entities;
      var docType;
      var retval;
      var elem;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "hc_staff");
      docType = doc.doctype;

      
	if(
	
	!(
	(builder.contentType == "text/html")
)

	) {
	assertNotNull("docTypeNotNull",docType);
entities = docType.entities;

      assertNotNull("entitiesNotNull",entities);
elem = doc.createElementNS("http://www.w3.org/1999/xhtml","br");
      
      try {
      retval = entities.setNamedItemNS(elem);
      fail("throw_HIER_OR_NO_MOD_ERR");
     
      } catch (ex) {
		  if (typeof(ex.code) != 'undefined') {      
       switch(ex.code) {
       case /* HIERARCHY_REQUEST_ERR */ 3 :
       break;
      case /* NO_MODIFICATION_ALLOWED_ERR */ 7 :
       break;
          default:
          throw ex;
          }
       } else { 
       throw ex;
        }
         }
        
	}
	
}




function runTest() {
   hc_entitiessetnameditemns1();
}
