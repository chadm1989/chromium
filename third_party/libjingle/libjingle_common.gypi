# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'nacl_untrusted_build%': 0,
    'webrtc_p2p': "../webrtc/p2p",
    'webrtc_xmpp': "../webrtc/libjingle/xmpp",
  },
  'sources': [
    '<(webrtc_p2p)/base/asyncstuntcpsocket.cc',
    '<(webrtc_p2p)/base/asyncstuntcpsocket.h',
    '<(webrtc_p2p)/base/basicpacketsocketfactory.cc',
    '<(webrtc_p2p)/base/basicpacketsocketfactory.h',
    '<(webrtc_p2p)/base/candidate.h',
    '<(webrtc_p2p)/base/common.h',
    '<(webrtc_p2p)/base/constants.cc',
    '<(webrtc_p2p)/base/constants.h',
    '<(webrtc_p2p)/base/dtlstransport.h',
    '<(webrtc_p2p)/base/dtlstransportchannel.cc',
    '<(webrtc_p2p)/base/dtlstransportchannel.h',
    '<(webrtc_p2p)/base/p2ptransport.cc',
    '<(webrtc_p2p)/base/p2ptransport.h',
    '<(webrtc_p2p)/base/p2ptransportchannel.cc',
    '<(webrtc_p2p)/base/p2ptransportchannel.h',
    '<(webrtc_p2p)/base/parsing.cc',
    '<(webrtc_p2p)/base/parsing.h',
    '<(webrtc_p2p)/base/port.cc',
    '<(webrtc_p2p)/base/port.h',
    '<(webrtc_p2p)/base/portallocator.cc',
    '<(webrtc_p2p)/base/portallocator.h',
    '<(webrtc_p2p)/base/portallocatorsessionproxy.cc',
    '<(webrtc_p2p)/base/portallocatorsessionproxy.h',
    '<(webrtc_p2p)/base/portproxy.cc',
    '<(webrtc_p2p)/base/portproxy.h',
    '<(webrtc_p2p)/base/pseudotcp.cc',
    '<(webrtc_p2p)/base/pseudotcp.h',
    '<(webrtc_p2p)/base/rawtransport.cc',
    '<(webrtc_p2p)/base/rawtransport.h',
    '<(webrtc_p2p)/base/rawtransportchannel.cc',
    '<(webrtc_p2p)/base/rawtransportchannel.h',
    '<(webrtc_p2p)/base/relayport.cc',
    '<(webrtc_p2p)/base/relayport.h',
    '<(webrtc_p2p)/base/session.cc',
    '<(webrtc_p2p)/base/session.h',
    '<(webrtc_p2p)/base/sessionclient.h',
    '<(webrtc_p2p)/base/sessiondescription.cc',
    '<(webrtc_p2p)/base/sessiondescription.h',
    '<(webrtc_p2p)/base/sessionid.h',
    '<(webrtc_p2p)/base/sessionmanager.cc',
    '<(webrtc_p2p)/base/sessionmanager.h',
    '<(webrtc_p2p)/base/sessionmessages.cc',
    '<(webrtc_p2p)/base/sessionmessages.h',
    '<(webrtc_p2p)/base/stun.cc',
    '<(webrtc_p2p)/base/stun.h',
    '<(webrtc_p2p)/base/stunport.cc',
    '<(webrtc_p2p)/base/stunport.h',
    '<(webrtc_p2p)/base/stunrequest.cc',
    '<(webrtc_p2p)/base/stunrequest.h',
    '<(webrtc_p2p)/base/tcpport.cc',
    '<(webrtc_p2p)/base/tcpport.h',
    '<(webrtc_p2p)/base/transport.cc',
    '<(webrtc_p2p)/base/transport.h',
    '<(webrtc_p2p)/base/transportchannel.cc',
    '<(webrtc_p2p)/base/transportchannel.h',
    '<(webrtc_p2p)/base/transportchannelimpl.h',
    '<(webrtc_p2p)/base/transportchannelproxy.cc',
    '<(webrtc_p2p)/base/transportchannelproxy.h',
    '<(webrtc_p2p)/base/transportdescription.cc',
    '<(webrtc_p2p)/base/transportdescription.h',
    '<(webrtc_p2p)/base/transportdescriptionfactory.cc',
    '<(webrtc_p2p)/base/transportdescriptionfactory.h',
    '<(webrtc_p2p)/base/turnport.cc',
    '<(webrtc_p2p)/base/turnport.h',
    '<(webrtc_p2p)/client/basicportallocator.cc',
    '<(webrtc_p2p)/client/basicportallocator.h',
    '<(webrtc_p2p)/client/httpportallocator.cc',
    '<(webrtc_p2p)/client/httpportallocator.h',
    '<(webrtc_p2p)/client/sessionmanagertask.h',
    '<(webrtc_p2p)/client/sessionsendtask.h',
    '<(webrtc_p2p)/client/socketmonitor.cc',
    '<(webrtc_p2p)/client/socketmonitor.h',
    '<(webrtc_xmpp)/asyncsocket.h',
    '<(webrtc_xmpp)/constants.cc',
    '<(webrtc_xmpp)/constants.h',
    '<(webrtc_xmpp)/jid.cc',
    '<(webrtc_xmpp)/jid.h',
    '<(webrtc_xmpp)/plainsaslhandler.h',
    '<(webrtc_xmpp)/prexmppauth.h',
    '<(webrtc_xmpp)/saslcookiemechanism.h',
    '<(webrtc_xmpp)/saslhandler.h',
    '<(webrtc_xmpp)/saslmechanism.cc',
    '<(webrtc_xmpp)/saslmechanism.h',
    '<(webrtc_xmpp)/saslplainmechanism.h',
    '<(webrtc_xmpp)/xmppclient.cc',
    '<(webrtc_xmpp)/xmppclient.h',
    '<(webrtc_xmpp)/xmppclientsettings.h',
    '<(webrtc_xmpp)/xmppengine.h',
    '<(webrtc_xmpp)/xmppengineimpl.cc',
    '<(webrtc_xmpp)/xmppengineimpl.h',
    '<(webrtc_xmpp)/xmppengineimpl_iq.cc',
    '<(webrtc_xmpp)/xmpplogintask.cc',
    '<(webrtc_xmpp)/xmpplogintask.h',
    '<(webrtc_xmpp)/xmppstanzaparser.cc',
    '<(webrtc_xmpp)/xmppstanzaparser.h',
    '<(webrtc_xmpp)/xmpptask.cc',
    '<(webrtc_xmpp)/xmpptask.h',
  ],
  'conditions': [
    ['OS=="win" and nacl_untrusted_build==0', {
      # Suppress warnings about WIN32_LEAN_AND_MEAN.
      'msvs_disabled_warnings': [ 4005, 4267 ],
    }, {
      'sources/': [
        ['exclude', '/win[a-z0-9]+\\.(h|cc)$'],
        ['exclude', '/schanneladapter\\.(h|cc)$'],
      ],
    }],
    ['os_posix!=1 and nacl_untrusted_build==0', {
      'sources/': [
        ['exclude', '/unix[a-z]+\\.(h|cc)$'],
      ],
    }],
    ['(OS!="mac" and OS!="ios") or nacl_untrusted_build==1', {
      'sources/': [
        ['exclude', '/mac[a-z]+\\.(h|cc)$'],
        ['exclude', '/scoped_autorelease_pool\\.(h|mm)$'],
      ],
    }],
    ['use_openssl!=1', {
      'sources/': [
        ['exclude', '/openssl[a-z]+\\.(h|cc)$'],
      ],
    }],
  ],
}
