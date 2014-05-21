/**
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This file requires these functions to be defined globally by someone else:
// function createPeerConnection(stun_server, useRtpDataChannel)
// function createOffer(peerConnection, constraints, callback)
// function receiveOffer(peerConnection, offer, constraints, callback)
// function receiveAnswer(peerConnection, answer, callback)

// Currently these functions are supplied by jsep01_call.js.

/**
 * We need a STUN server for some API calls.
 * @private
 */
var STUN_SERVER = 'stun.l.google.com:19302';

/**
 * This object represents the call.
 * @private
 */
var gPeerConnection = null;

/**
 * If true, any created peer connection will use RTP data
 * channels. Otherwise it will use SCTP data channels.
 */
var gUseRtpDataChannels = true;

/**
 * This stores ICE candidates generated on this side.
 * @private
 */
var gIceCandidates = [];

// Public interface to tests. These are expected to be called with
// ExecuteJavascript invocations from the browser tests and will return answers
// through the DOM automation controller.

/**
 * Creates a peer connection. Must be called before most other public functions
 * in this file.
 */
function preparePeerConnection() {
  if (gPeerConnection != null)
    throw failTest('creating peer connection, but we already have one.');

  gPeerConnection = createPeerConnection(STUN_SERVER, gUseRtpDataChannels);
  returnToTest('ok-peerconnection-created');
}

/**
 * Asks this page to create a local offer.
 *
 * Returns a string on the format ok-(JSON encoded session description).
 *
 * @param {!object} constraints Any createOffer constraints.
 */
function createLocalOffer(constraints) {
  if (gPeerConnection == null)
    throw failTest('Negotiating call, but we have no peer connection.');

  // TODO(phoglund): move jsep01.call stuff into this file and remove need
  // of the createOffer method, etc.
  createOffer(gPeerConnection, constraints, function(localOffer) {
    returnToTest('ok-' + JSON.stringify(localOffer));
  });
}

/**
 * Asks this page to accept an offer and generate an answer.
 *
 * Returns a string on the format ok-(JSON encoded session description).
 *
 * @param {!string} sessionDescJson A JSON-encoded session description of type
 *     'offer'.
 * @param {!object} constraints Any createAnswer constraints.
 */
function receiveOfferFromPeer(sessionDescJson, constraints) {
  if (gPeerConnection == null)
    throw failTest('Receiving offer, but we have no peer connection.');

  offer = parseJson_(sessionDescJson);
  if (!offer.type)
    failTest('Got invalid session description from peer: ' + sessionDescJson);
  if (offer.type != 'offer')
    failTest('Expected to receive offer from peer, got ' + offer.type);

  receiveOffer(gPeerConnection, offer , constraints, function(answer) {
    returnToTest('ok-' + JSON.stringify(answer));
  });
}

/**
 * Asks this page to accept an answer generated by the peer in response to a
 * previous offer by this page
 *
 * Returns a string ok-accepted-answer on success.
 *
 * @param {!string} sessionDescJson A JSON-encoded session description of type
 *     'answer'.
 */
function receiveAnswerFromPeer(sessionDescJson) {
  if (gPeerConnection == null)
    throw failTest('Receiving offer, but we have no peer connection.');

  answer = parseJson_(sessionDescJson);
  if (!answer.type)
    failTest('Got invalid session description from peer: ' + sessionDescJson);
  if (answer.type != 'answer')
    failTest('Expected to receive answer from peer, got ' + answer.type);

  receiveAnswer(gPeerConnection, answer, function() {
    returnToTest('ok-accepted-answer');
  });
}

/**
 * Adds the local stream to the peer connection. You will have to re-negotiate
 * the call for this to take effect in the call.
 */
function addLocalStream() {
  if (gPeerConnection == null)
    throw failTest('adding local stream, but we have no peer connection.');

  addLocalStreamToPeerConnection(gPeerConnection);
  returnToTest('ok-added');
}

/**
 * Loads a file with WebAudio and connects it to the peer connection.
 *
 * The loadAudioAndAddToPeerConnection will return ok-added to the test when
 * the sound is loaded and added to the peer connection. The sound will start
 * playing when you call playAudioFile.
 *
 * @param url URL pointing to the file to play. You can assume that you can
 *     serve files from the repository's file system. For instance, to serve a
 *     file from chrome/test/data/pyauto_private/webrtc/file.wav, pass in a path
 *     relative to this directory (e.g. ../pyauto_private/webrtc/file.wav).
 */
function addAudioFile(url) {
  if (gPeerConnection == null)
    throw failTest('adding audio file, but we have no peer connection.');

  loadAudioAndAddToPeerConnection(url, gPeerConnection);
}

/**
 * Mixes the local audio stream with an audio file through WebAudio.
 *
 * You must have successfully requested access to the user's microphone through
 * getUserMedia before calling this function (see getUserMedia.js).
 * Additionally, you must have loaded an audio file to mix with.
 *
 * When playAudioFile is called, WebAudio will effectively mix the user's
 * microphone input with the previously loaded file and feed that into the
 * peer connection.
 */
function mixLocalStreamWithPreviouslyLoadedAudioFile() {
  if (gPeerConnection == null)
    throw failTest('trying to mix in stream, but we have no peer connection.');
  if (getLocalStream() == null)
    throw failTest('trying to mix in stream, but we have no stream to mix in.');

  mixLocalStreamIntoPeerConnection(gPeerConnection, getLocalStream());
}

/**
 * Must be called after addAudioFile.
 */
function playAudioFile() {
  if (gPeerConnection == null)
    throw failTest('trying to play file, but we have no peer connection.');

  playPreviouslyLoadedAudioFile(gPeerConnection);
  returnToTest('ok-playing');
}

/**
 * Hangs up a started call. Returns ok-call-hung-up on success.
 */
function hangUp() {
  if (gPeerConnection == null)
    throw failTest('hanging up, but has no peer connection');
  gPeerConnection.close();
  gPeerConnection = null;
  returnToTest('ok-call-hung-up');
}

/**
 * Retrieves all ICE candidates generated on this side. Must be called after
 * ICE candidate generation is triggered (for instance by running a call
 * negotiation). This function will wait if necessary if we're not done
 * generating ICE candidates on this side.
 *
 * Returns a JSON-encoded array of RTCIceCandidate instances to the test.
 */
function getAllIceCandidates() {
  if (gPeerConnection == null)
    throw failTest('Trying to get ICE candidates, but has no peer connection.');

  if (gPeerConnection.iceGatheringState != 'complete') {
    console.log('Still ICE gathering - waiting...');
    setTimeout(getAllIceCandidates, 100);
    return;
  }

  returnToTest(JSON.stringify(gIceCandidates));
}

/**
 * Receives ICE candidates from the peer.
 *
 * Returns ok-received-candidates to the test on success.
 *
 * @param iceCandidatesJson a JSON-encoded array of RTCIceCandidate instances.
 */
function receiveIceCandidates(iceCandidatesJson) {
  if (gPeerConnection == null)
    throw failTest('Received ICE candidate, but has no peer connection');

  var iceCandidates = parseJson_(iceCandidatesJson);
  if (!iceCandidates.length)
    throw failTest('Received invalid ICE candidate list from peer: ' +
      iceCandidatesJson);

  iceCandidates.forEach(function(iceCandidate) {
    if (!iceCandidate.candidate)
      failTest('Received invalid ICE candidate from peer: ' +
        iceCandidatesJson);

    gPeerConnection.addIceCandidate(new RTCIceCandidate(iceCandidate));
  });

  returnToTest('ok-received-candidates');
}

// Public interface to signaling implementations, such as JSEP.

/**
 * Enqueues an ICE candidate for sending to the peer.
 *
 * @param {!RTCIceCandidate} The ICE candidate to send.
 */
function sendIceCandidate(message) {
  gIceCandidates.push(message);
}

/**
 * Parses JSON-encoded session descriptions and ICE candidates.
 * @private
 */
function parseJson_(json) {
  // Escape since the \r\n in the SDP tend to get unescaped.
  jsonWithEscapedLineBreaks = json.replace(/\r\n/g, '\\r\\n');
  try {
    return JSON.parse(jsonWithEscapedLineBreaks);
  } catch (exception) {
    failTest('Failed to parse JSON: ' + jsonWithEscapedLineBreaks + ', got ' +
             exception);
  }
}
