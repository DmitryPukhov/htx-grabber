# htx-grabber
Connect to https://htx.com exchange, subscribe to public websocket data and grab received data to local files. The data is free on htx, no need for API key.

## Configuration
Set **LISTEN_CHANNELS** environment variable to comma separated channel names to grab.<br/>
**Example:**<br/>
LISTEN_CHANNELS=market.btcusdt.depth.step0,market.btcusdt.depth.bbo
<br/>
Possible data channel names for SWAP market:
https://www.htx.com/en-us/opend/newApiPages/?id=8cb7c385-77b5-11ed-9966-0242ac110003
