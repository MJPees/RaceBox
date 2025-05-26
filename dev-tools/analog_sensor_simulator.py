import asyncio
import websockets

SMARTRACE_HOST_IP = "100.115.92.22"
SMARTRACE_PORT = 45215


async def main():
    uri = f"ws://{SMARTRACE_HOST_IP}:{SMARTRACE_PORT}"
    async with websockets.connect(uri) as smartrace_websocket:
        print(f"Connected to SmartRace at {uri}")
        await smartrace_websocket.send(
            '{"type":"controller_set","data":{"controller_id":"Z"}}'
        )

        async for message in smartrace_websocket:
            print(f"Received from SmartRace: {message}")


if __name__ == "__main__":
    asyncio.run(main())
