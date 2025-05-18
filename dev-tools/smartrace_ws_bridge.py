import asyncio
import websockets

SMARTRACE_HOST_IP = "100.115.92.22"
SMARTRACE_PORT = 45215
BRIDGE_HOST_IP = "0.0.0.0"
BRIDGE_HOST_PORT = 50780


async def bridge_client_to_smartrace(websocket, path):
    uri = f"ws://{SMARTRACE_HOST_IP}:{SMARTRACE_PORT}"
    async with websockets.connect(uri) as smartrace_websocket:
        print(f"Connected to SmartRace at {uri}")

        async def forward_to_client():
            async for message in smartrace_websocket:
                print(f"Received from SmartRace: {message}")
                await websocket.send(message)

        async def forward_to_smartrace():
            async for message in websocket:
                print(f"Received from client: {message}")
                await smartrace_websocket.send(message)
                print("Message forwarded to SmartRace")

        await asyncio.gather(forward_to_client(), forward_to_smartrace())


async def start_bridge_server():
    server = await websockets.serve(
        bridge_client_to_smartrace, BRIDGE_HOST_IP, BRIDGE_HOST_PORT
    )
    print(f"WebSocket server started at ws://{BRIDGE_HOST_IP}:{BRIDGE_HOST_PORT}")
    await server.wait_closed()


if __name__ == "__main__":
    asyncio.run(start_bridge_server())
