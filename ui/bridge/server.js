const express = require("express");
const net = require("net");
const cors = require("cors");

const app = express();
app.use(express.json({ limit: "50mb" }));
app.use(cors());

const OFS_HOST = "127.0.0.1";
const OFS_PORT = 8080;

app.post("/send", (req, res) => {
    const json = JSON.stringify(req.body) + "\n";

    const client = new net.Socket();
    client.connect(OFS_PORT, OFS_HOST, () => {
        client.write(json);
    });

    client.on("data", data => {
        res.send(data.toString());
        client.destroy();
    });

    client.on("error", err => {
        res.status(500).send({ error: err.message });
    });
});

app.listen(4000, () => {
    console.log("Bridge is running at http://localhost:4000");
});