// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Autofill Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE([
    ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
    ROOT_PATH +
        'chrome/test/data/webui/settings/passwords_and_autofill_fake_data.js',
    ROOT_PATH + 'ui/webui/resources/js/load_time_data.js',
]);

/**
 * Test implementation.
 * @implements {settings.address.CountryDetailManager}
 * @constructor
 */
function CountryDetailManagerTestImpl() {}
CountryDetailManagerTestImpl.prototype = {
  /** @override */
  getCountryList: function() {
    return new Promise(function(resolve) {
      resolve([
          {name: 'United States', countryCode: 'US'},  // Default test country.
          {name: 'Israel', countryCode: 'IL'},
          {name: 'United Kingdom', countryCode: 'GB'},
      ]);
    });
  },

  /** @override */
  getAddressFormat: function(countryCode) {
    return new Promise(function(resolve) {
      chrome.autofillPrivate.getAddressComponents(countryCode, resolve);
    });
  },
};

/**
 * Will call |loopBody| for each item in |items|. Will only move to the next
 * item after the promise from |loopBody| resolves.
 * @param {!Array<Object>} items
 * @param {!function(!Object):!Promise} loopBody
 * @return {!Promise}
 */
function asyncForEach(items, loopBody) {
  return new Promise(function(finish) {
    var index = 0;

    function loop() {
      var item = items[index++];
      if (item)
        loopBody(item).then(loop);
      else
        finish();
    };

    loop();
  });
}

/**
 * Resolves the promise after the element fires the expected event. Will add and
 * remove the listener so it is only triggered once. |causeEvent| is called
 * after adding a listener to make sure that the event is captured.
 * @param {!Element} element
 * @param {string} eventName
 * @param {function():void} causeEvent
 * @return {!Promise}
 */
function expectEvent(element, eventName, causeEvent) {
  return new Promise(function(resolve) {
    var callback = function() {
      element.removeEventListener(eventName, callback);
      resolve.apply(this, arguments);
    };
    element.addEventListener(eventName, callback);
    causeEvent();
  });
}

/**
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsAutofillSectionBrowserTest() {}

SettingsAutofillSectionBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload:
      'chrome://md-settings/passwords_and_forms_page/autofill_section.html',

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH),

  /**
   * TODO(hcarmona): Increases speed, but disables A11y checks. Enable checks
   * when we "accessibilityIssuesAreErrors: true" for all tests.
   * @override
   */
  runAccessibilityChecks: false,

  i18nStrings: {
    addAddressTitle: 'add-title',
    addCreditCardTitle: 'add-title',
    editAddressTitle: 'edit-title',
    editCreditCardTitle: 'edit-title',
  },

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);

    // Test is run on an individual element that won't have a page language.
    this.accessibilityAuditConfig.auditRulesToIgnore.push('humanLangMissing');

    // Faking 'strings.js' for this test.
    loadTimeData.data = this.i18nStrings;

    settings.address.CountryDetailManagerImpl.instance_ =
        new CountryDetailManagerTestImpl();
  },

  /**
   * Allow the iron-list to be sized properly.
   * @param {!Object} autofillSection
   * @private
   */
  flushAutofillSection_: function(autofillSection) {
    autofillSection.$.addressList.notifyResize();
    autofillSection.$.creditCardList.notifyResize();
    Polymer.dom.flush();
  },

  /**
   * Creates the autofill section for the given lists.
   * @param {!Array<!chrome.passwordsPrivate.PasswordUiEntry>} passwordList
   * @param {!Array<!chrome.passwordsPrivate.ExceptionPair>} exceptionList
   * @return {!Object}
   * @private
   */
  createAutofillSection_: function(addresses, creditCards) {
    var section = document.createElement('settings-autofill-section');
    section.addresses = addresses;
    section.creditCards = creditCards;
    document.body.appendChild(section);
    this.flushAutofillSection_(section);
    return section;
  },

  /**
   * Creates the Edit Address dialog and fulfills the promise when the dialog
   * has actually opened.
   * @param {!chrome.autofillPrivate.AddressEntry} address
   * @return {!Promise<Object>}
   */
  createAddressDialog_: function(address) {
    return new Promise(function(resolve) {
      var section = document.createElement('settings-address-edit-dialog');
      document.body.appendChild(section);
      var onOpen = function() {
        resolve(section);
      };
      section.addEventListener('iron-overlay-opened', onOpen);

      // |setTimeout| allows the dialog to async get the list of countries
      // before running any tests.
      window.setTimeout(function() {
        section.open(address);  // Opening the dialog will add the item.
        Polymer.dom.flush();
      }, 0);
    });
  },

  /**
   * Creates the Edit Credit Card dialog.
   * @param {!chrome.autofillPrivate.CreditCardEntry} creditCardItem
   * @return {!Object}
   */
  createCreditCardDialog_: function(creditCardItem) {
    var section = document.createElement('settings-credit-card-edit-dialog');
    document.body.appendChild(section);
    section.open(creditCardItem);  // Opening the dialog will add the item.
    Polymer.dom.flush();
    return section;
  },
};

TEST_F('SettingsAutofillSectionBrowserTest', 'CreditCardTests', function() {
  var self = this;

  suite('AutofillSection', function() {
    test('verifyCreditCardCount', function() {
      var creditCards = [
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
        FakeDataMaker.creditCardEntry(),
      ];

      var section = self.createAutofillSection_([], creditCards);

      assertTrue(!!section);
      var creditCardList = section.$.creditCardList;
      assertTrue(!!creditCardList);
      assertEquals(creditCards, creditCardList.items);
      // +1 for the template element.
      assertEquals(creditCards.length + 1, creditCardList.children.length);
    });

    test('verifyCreditCardFields', function() {
      var creditCard = FakeDataMaker.creditCardEntry();
      var section = self.createAutofillSection_([], [creditCard]);
      var creditCardList = section.$.creditCardList;
      var row = creditCardList.children[1];  // Skip over the template.
      assertTrue(!!row);

      assertEquals(creditCard.metadata.summaryLabel,
                   row.querySelector('#creditCardLabel').textContent);
      assertEquals(creditCard.expirationMonth + '/' + creditCard.expirationYear,
                   row.querySelector('#creditCardExpiration').textContent);
    });

    test('verifyAddVsEditCreditCardTitle', function() {
      var newCreditCard = FakeDataMaker.emptyCreditCardEntry();
      var newCreditCardDialog = self.createCreditCardDialog_(newCreditCard);
      var oldCreditCard = FakeDataMaker.creditCardEntry();
      var oldCreditCardDialog = self.createCreditCardDialog_(oldCreditCard);

      assertNotEquals(oldCreditCardDialog.title_, newCreditCardDialog.title_);
      assertNotEquals('', newCreditCardDialog.title_);
      assertNotEquals('', oldCreditCardDialog.title_);
    });

    test('verifyExpiredCreditCardYear', function() {
      var creditCard = FakeDataMaker.creditCardEntry();

      // 2015 is over unless time goes wobbly.
      var twentyFifteen = 2015;
      creditCard.expirationYear = twentyFifteen.toString();

      var creditCardDialog = self.createCreditCardDialog_(creditCard);
      var selectableYears = creditCardDialog.$.yearList.items;
      var firstSelectableYear = selectableYears[0];
      var lastSelectableYear = selectableYears[selectableYears.length - 1];

      var now = new Date();
      var maxYear = now.getFullYear() + 9;

      assertEquals('2015', firstSelectableYear.textContent);
      assertEquals(maxYear.toString(), lastSelectableYear.textContent);
    });

    test('verifyVeryFutureCreditCardYear', function() {
      var creditCard = FakeDataMaker.creditCardEntry();

      // Expiring 20 years from now is unusual.
      var now = new Date();
      var farFutureYear = now.getFullYear() + 20;
      creditCard.expirationYear = farFutureYear.toString();

      var creditCardDialog = self.createCreditCardDialog_(creditCard);
      var selectableYears = creditCardDialog.$.yearList.items;
      var firstSelectableYear = selectableYears[0];
      var lastSelectableYear = selectableYears[selectableYears.length - 1];

      assertEquals(now.getFullYear().toString(),
          firstSelectableYear.textContent);
      assertEquals(farFutureYear.toString(), lastSelectableYear.textContent);
    });

    test('verifyVeryNormalCreditCardYear', function() {
      var creditCard = FakeDataMaker.creditCardEntry();

      // Expiring 2 years from now is not unusual.
      var now = new Date();
      var nearFutureYear = now.getFullYear() + 2;
      creditCard.expirationYear = nearFutureYear.toString();
      var maxYear = now.getFullYear() + 9;

      var creditCardDialog = self.createCreditCardDialog_(creditCard);
      var selectableYears = creditCardDialog.$.yearList.items;
      var firstSelectableYear = selectableYears[0];
      var lastSelectableYear = selectableYears[selectableYears.length - 1];

      assertEquals(now.getFullYear().toString(),
          firstSelectableYear.textContent);
      assertEquals(maxYear.toString(), lastSelectableYear.textContent);
    });

    // Test will timeout if event is not received.
    test('verifySaveCreditCardEdit', function(done) {
      var creditCard = FakeDataMaker.emptyCreditCardEntry();
      var creditCardDialog = self.createCreditCardDialog_(creditCard);

      creditCardDialog.addEventListener('save-credit-card', function(event) {
        assertEquals(creditCard.guid, event.detail.guid);
        done();
      });

      MockInteractions.tap(creditCardDialog.$.saveButton);
    });

    test('verifyCancelCreditCardEdit', function(done) {
      var creditCard = FakeDataMaker.emptyCreditCardEntry();
      var creditCardDialog = self.createCreditCardDialog_(creditCard);

      creditCardDialog.addEventListener('save-credit-card', function() {
        // Fail the test because the save event should not be called when cancel
        // is clicked.
        assertTrue(false);
        done();
      });

      creditCardDialog.addEventListener('iron-overlay-closed', function() {
        // Test is |done| in a timeout in order to ensure that
        // 'save-credit-card' is NOT fired after this test.
        window.setTimeout(done, 100);
      });

      MockInteractions.tap(creditCardDialog.$.cancelButton);
    });
  });

  mocha.run();
});

TEST_F('SettingsAutofillSectionBrowserTest', 'AddressTests', function() {
  var self = this;

  suite('AutofillSection', function() {
    test('verifyAddressCount', function() {
      var addresses = [
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
        FakeDataMaker.addressEntry(),
      ];

      var section = self.createAutofillSection_(addresses, []);

      assertTrue(!!section);
      var addressList = section.$.addressList;
      assertTrue(!!addressList);
      assertEquals(addresses, addressList.items);
      // +1 for the template element.
      assertEquals(addresses.length + 1, addressList.children.length);
    });

    test('verifyAddressFields', function() {
      var address = FakeDataMaker.addressEntry();
      var section = self.createAutofillSection_([address], []);
      var addressList = section.$.addressList;
      var row = addressList.children[1];  // Skip over the template.
      assertTrue(!!row);

      var addressSummary = address.metadata.summaryLabel +
                           address.metadata.summarySublabel;

      assertEquals(addressSummary,
                   row.querySelector('#addressSummary').textContent);
    });

    test('verifyAddAddressDialog', function() {
      return self.createAddressDialog_(
          FakeDataMaker.emptyAddressEntry()).then(function(dialog) {
        var title = dialog.$$('.title');
        assertEquals(self.i18nStrings.addAddressTitle, title.textContent);
        // Shouldn't be possible to save until something is typed in.
        assertTrue(dialog.$.saveButton.disabled);
      });
    });

    test('verifyEditAddressDialog', function() {
      return self.createAddressDialog_(
          FakeDataMaker.addressEntry()).then(function(dialog) {
        var title = dialog.$$('.title');
        assertEquals(self.i18nStrings.editAddressTitle, title.textContent);
        // Should be possible to save when editing because fields are populated.
        assertFalse(dialog.$.saveButton.disabled);
      });
    });

    test('verifyCountryIsSaved', function() {
      var address = FakeDataMaker.emptyAddressEntry();
      return self.createAddressDialog_(address).then(function(dialog) {
        assertEquals(undefined, dialog.$.countryList.selected);
        assertEquals(undefined, address.countryCode);
        dialog.$.countryList.selected = 'US';
        Polymer.dom.flush();
        assertEquals('US', dialog.$.countryList.selected);
        assertEquals('US', address.countryCode);
      });
    });

    test('verifyPhoneAndEmailAreSaved', function() {
      var address = FakeDataMaker.emptyAddressEntry();
      return self.createAddressDialog_(address).then(function(dialog) {
        assertEquals('', dialog.$.phoneInput.value);
        assertFalse(!!(address.phoneNumbers && address.phoneNumbers[0]));

        assertEquals('', dialog.$.emailInput.value);
        assertFalse(!!(address.emailAddresses && address.emailAddresses[0]));

        var phoneNumber = '(555) 555-5555';
        var emailAddress = 'no-reply@chromium.org';

        dialog.$.phoneInput.value = phoneNumber;
        dialog.$.emailInput.value = emailAddress;

        Polymer.dom.flush();

        assertEquals(phoneNumber, dialog.$.phoneInput.value);
        assertEquals(phoneNumber, address.phoneNumbers[0]);

        assertEquals(emailAddress, dialog.$.emailInput.value);
        assertEquals(emailAddress, address.emailAddresses[0]);
      });
    });

    // Test will set a value of 'foo' in each text field and verify that the
    // save button is enabled, then it will clear the field and verify that the
    // save button is disabled. Test passes after all elements have been tested.
    test('verifySaveIsNotClickableIfAllInputFieldsAreEmpty', function() {
      return self.createAddressDialog_(
          FakeDataMaker.emptyAddressEntry()).then(function(dialog) {
        var saveButton = dialog.$.saveButton;
        var testElements =
            dialog.$.dialog.querySelectorAll('paper-input,paper-textarea');

        // Default country is 'US' expecting: Name, Organization,
        // Street address, City, State, ZIP code, Phone, and Email.
        assertEquals(8, testElements.length);

        return asyncForEach(testElements, function(element) {
          return expectEvent(dialog, 'on-update-can-save', function() {
            assertTrue(saveButton.disabled);
            element.value = 'foo';
          }).then(function() {
            return expectEvent(dialog, 'on-update-can-save', function() {
              assertFalse(saveButton.disabled);
              element.value = '';
            });
          }).then(function() {
            assertTrue(saveButton.disabled);
          });
        });
      });
    });

    // Setting the country should allow the address to be saved.
    test('verifySaveIsNotClickableIfCountryNotSet', function() {
      return self.createAddressDialog_(
          FakeDataMaker.emptyAddressEntry()).then(function(dialog) {
        var saveButton = dialog.$.saveButton;
        var countries = dialog.$.countryList;

        return expectEvent(dialog, 'on-update-can-save', function() {
          assertTrue(saveButton.disabled);
          countries.selected = 'US';
        }).then(function() {
            assertFalse(saveButton.disabled);
            countries.selected = '';
        }).then(function() {
          assertTrue(saveButton.disabled);
        });
      });
    });

    // Test will timeout if save-address event is not fired.
    test('verifyDefaultCountryIsAppliedWhenSaving', function() {
      var address = FakeDataMaker.emptyAddressEntry();
      address.companyName = 'Google';
      return self.createAddressDialog_(address).then(function(dialog) {
        return expectEvent(dialog, 'save-address', function() {
          // Verify |countryCode| is not set.
          assertEquals(undefined, address.countryCode);
          MockInteractions.tap(dialog.$.saveButton);
        }).then(function(event) {
          // 'US' is the default country for these tests.
          assertEquals('US', event.detail.countryCode);
        });
      });
    });

    test('verifyCancelDoesNotSaveAddress', function(done) {
      self.createAddressDialog_(
          FakeDataMaker.addressEntry()).then(function(dialog) {
        dialog.addEventListener('save-address', function() {
          // Fail the test because the save event should not be called when
          // cancel is clicked.
          assertTrue(false);
          done();
        });

        dialog.addEventListener('iron-overlay-closed', function() {
          // Test is |done| in a timeout in order to ensure that
          // 'save-address' is NOT fired after this test.
          window.setTimeout(done, 100);
        });

        MockInteractions.tap(dialog.$.cancelButton);
      });
    });
  });

  mocha.run();
});

TEST_F('SettingsAutofillSectionBrowserTest', 'AddressLocaleTests', function() {
  var self = this;

  suite('AutofillSection', function() {
    // US address has 3 fields on the same line.
    test('verifyEditingUSAddress', function() {
      var address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = [ 'Name' ];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'City';
      address.addressLevel1 = 'State';
      address.postalCode = 'ZIP code';
      address.countryCode = 'US';
      address.phoneNumbers = [ 'Phone' ];
      address.emailAddresses = [ 'Email' ];

      return self.createAddressDialog_(address).then(function(dialog) {
        var rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(6, rows.length);

        // Name
        var row = rows[0];
        var cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // City, State, ZIP code
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(3, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        assertEquals(address.addressLevel1, cols[1].value);
        assertEquals(address.postalCode, cols[2].value);
        // Country
        row = rows[4];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals('United States', cols[0].value);
        // Phone, Email
        row = rows[5];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // GB address has 1 field per line for all lines that change.
    test('verifyEditingGBAddress', function() {
      var address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = [ 'Name' ];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'Post town';
      address.addressLevel1 = 'County';
      address.postalCode = 'Postal code';
      address.countryCode = 'GB';
      address.phoneNumbers = [ 'Phone' ];
      address.emailAddresses = [ 'Email' ];

      return self.createAddressDialog_(address).then(function(dialog) {
        var rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(8, rows.length);

        // Name
        var row = rows[0];
        var cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // Post Town
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        // County
        row = rows[4];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLevel1, cols[0].value);
        // Postal code
        row = rows[5];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.postalCode, cols[0].value);
        // Country
        row = rows[6];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals('United Kingdom', cols[0].value);
        // Phone, Email
        row = rows[7];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // IL address has 2 fields on the same line and is an RTL locale.
    // RTL locale shouldn't affect this test.
    test('verifyEditingILAddress', function() {
      var address = FakeDataMaker.emptyAddressEntry();

      address.fullNames = [ 'Name' ];
      address.companyName = 'Organization';
      address.addressLines = 'Street address';
      address.addressLevel2 = 'City';
      address.postalCode = 'Postal code';
      address.countryCode = 'IL';
      address.phoneNumbers = [ 'Phone' ];
      address.emailAddresses = [ 'Email' ];

      return self.createAddressDialog_(address).then(function(dialog) {
        var rows = dialog.$.dialog.querySelectorAll('.address-row');
        assertEquals(6, rows.length);

        // Name
        var row = rows[0];
        var cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.fullNames[0], cols[0].value);
        // Organization
        row = rows[1];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.companyName, cols[0].value);
        // Street address
        row = rows[2];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals(address.addressLines, cols[0].value);
        // City, Postal code
        row = rows[3];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.addressLevel2, cols[0].value);
        assertEquals(address.postalCode, cols[1].value);
        // Country
        row = rows[4];
        cols = row.querySelectorAll('.address-column');
        assertEquals(1, cols.length);
        assertEquals('Israel', cols[0].value);
        // Phone, Email
        row = rows[5];
        cols = row.querySelectorAll('.address-column');
        assertEquals(2, cols.length);
        assertEquals(address.phoneNumbers[0], cols[0].value);
        assertEquals(address.emailAddresses[0], cols[1].value);
      });
    });

    // US has an extra field 'State'. Validate that this field is
    // persisted when switching to IL then back to US.
    test('verifyAddressPersistanceWhenSwitchingCountries', function() {
      var address = FakeDataMaker.emptyAddressEntry();
      address.countryCode = 'US';

      return self.createAddressDialog_(address).then(function(dialog) {
        var city = 'Los Angeles';
        var state = 'CA';
        var zip = '90291';

        return expectEvent(dialog, 'on-update-address-wrapper', function() {
          // US:
          var rows = dialog.$.dialog.querySelectorAll('.address-row');
          assertEquals(6, rows.length);

          // City, State, ZIP code
          var row = rows[3];
          var cols = row.querySelectorAll('.address-column');
          assertEquals(3, cols.length);
          cols[0].value = city;
          cols[1].value = state;
          cols[2].value = zip;

          dialog.$.countryList.selected = 'IL';
        }).then(function() {
          return expectEvent(dialog, 'on-update-address-wrapper', function() {
            // IL:
            rows = dialog.$.dialog.querySelectorAll('.address-row');
            assertEquals(6, rows.length);

            // City, Postal code
            row = rows[3];
            cols = row.querySelectorAll('.address-column');
            assertEquals(2, cols.length);
            assertEquals(city, cols[0].value);
            assertEquals(zip, cols[1].value);

            dialog.$.countryList.selected = 'US';
          });
        }).then(function() {
          // US:
          var rows = dialog.$.dialog.querySelectorAll('.address-row');
          assertEquals(6, rows.length);

          // City, State, ZIP code
          row = rows[3];
          cols = row.querySelectorAll('.address-column');
          assertEquals(3, cols.length);
          assertEquals(city, cols[0].value);
          assertEquals(state, cols[1].value);
          assertEquals(zip, cols[2].value);
        });
      });
    });
  });

  mocha.run();
});
