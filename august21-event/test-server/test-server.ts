import type { ServerWebSocket, Server, TLSWebSocketServeOptions } from "bun"
import { DataReader, DataWriter } from "dataproto"
import repl from "basic-repl"

type ClientData = {
    x: 0,
    y: 0,
    phase: "loadingscreen"
}

interface TestServer extends Server {
    clients: Set<ServerWebSocket<ClientData>>
}

const serverOptions:TLSWebSocketServeOptions<ClientData> = {
    async fetch(req: Request, server: Server) {
        server.upgrade(req, {
            data: {
                x: 0,
                y: 0
            },
            headers: {
                "Access-Control-Allow-Origin": "*",
                "Access-Control-Allow-Credentials": "true",
            }
        })

        return undefined
    },
    websocket: {
        async open(ws: ServerWebSocket<ClientData>) {
            ws.subscribe("all")
            wss.clients.add(ws)
        },
        async message(ws:ServerWebSocket<ClientData>, data:string|Buffer) {

        },
        async close(ws:ServerWebSocket<ClientData>, code: number, message: string) {
            wss.clients.delete(ws)
        },
        perMessageDeflate: false,
    },
    port: 8088
}
const bunServer = Bun.serve<ClientData>(serverOptions)
// @ts-ignore
bunServer.clients = new Set<ServerWebSocket<ClientData>>()
// @ts-ignore
const wss:TestServer = bunServer
console.log("Server started on", wss.url.href)

const infoInterval = setInterval(() => {
    const infoBuffer = new DataWriter()
    infoBuffer.uint8(1)
    infoBuffer.uint32(wss.clients.size)
    wss.publish("all", infoBuffer.build())
}, 1000)


function start() {
    clearInterval(infoInterval)
    const buffer = new DataWriter()
    buffer.uint8(2)
    wss.publish("all", buffer.build())
}

process.on("SIGINT", () => {
    process.exit(0)
})

repl("[server] ", async (source:string) => {
    try {
        eval(`(async function(){console.log(${source ? "await " + source : "undefined"})})()`)
    }
    catch(e) {
        console.error(e)
    }
})