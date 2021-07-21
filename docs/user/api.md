---
sort: 5
---

# REST API

A REST API is available at [http://127.0.0.1:5000](http://127.0.0.1:5000) (the port can be changed thanks to `--api-port`) when speculos is running. The specification of the API can be found at this URL, or from the swagger.io [demo website](https://petstore.swagger.io/?url=https://raw.githubusercontent.com/LedgerHQ/speculos/master/speculos/api/static/swagger/swagger.json).

This API is meant to be used in test environments to automate actions on the device such as:

- Taking screenshot
- Pressing buttons
- Setting or updating [automation rules](automation.md)
- etc.

## Usage

For instance, pressing the left button is as simple as:

```shell
curl -d '{"action":"press-and-release"}' http://127.0.0.1:5000/button/left
```

and taking a screenshot of the device:

```shell
curl -o screenshot.png http://127.0.0.1:5000/screenshot
```

## Web UI

There is a web user interface running directly on [http://127.0.0.1:5000](http://127.0.0.1:5000), which communicates with the API:

![screenshot btc nano s](../screenshot-api-nanos-btc.png)
