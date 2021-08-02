const http = require('http')
const https = require('https')
const querystring = require('querystring')
const {URL} = require("url")

class HttpClient {
    constructor(baseUrl, isFormType, options) {
        const url = new URL(baseUrl)
        const isSSL = url.protocol == "https:"
        this.baseUrl = baseUrl
        this.hc = isSSL ? https : http
		this.defaultOption = options ? { ...options } : {}
        this.defaultOption.host = url.hostname
        if (url.port)
			this.defaultOption.port = url.port
        this.isFormType = isFormType
        this.defaultHeaders = {}
        this.token = null
        this.debug = true
    }

    setDebug(flag) {
        this.debug = flag
    }

    addDefaultHeader(name, value) {
        this.defaultHeaders[name] = value
    }

    removeDefaultHeader(name) {
        delete this.defaultHeaders[name]
    }

    async request(method, path, urlParam, param, headers) {
        const url = urlParam ? this.baseUrl + path + "?" + querystring.stringify(urlParam) : this.baseUrl + path
        const data = param ? this.isFormType ? querystring.stringify(param) : JSON.stringify(param) : undefined
        const body = data ? Buffer.from(data) : undefined
        const httpOption = { ...this.defaultOption, path, method, headers: { ...this.defaultHeaders }}

        if (body) {
            httpOption.headers["Content-type"] = this.isFormType ? "application/x-www-form-urlencoded" : "application/json"
            httpOption.headers["Content-Length"] = body.length
        }

        if (this.token)
            httpOption.headers["authorization"] = "Bearer " + this.token

        if (headers)
            for (let k in headers)
                httpOption.headers[k] = headers[k]

        const hc = this.hc
        const debug = this.debug
        return new Promise((resolve, reject) => {
            const req = hc.request(httpOption, (res) => {
                let result = { status: res.statusCode, message: res.statusMessage, data: "" }
                res.on('data', (chunk) => result.data += chunk);
                res.on('end', () => {
                    if (debug)
                        console.log("访问[" + url + "]结果:", result, "\n\n")
                    resolve(result)
                })
                req.on('error', (e) => {
                    if (debug)
                        console.log("访问[" + url + "] 发生异常: " + e.message)
                    reject(e)
                })
            })
            if (body) req.write(body)
            req.end()
        })
    }

    async get(path, urlParam, headers) {
        return this.request("GET", path, urlParam, null, headers)
    }

    async post(path, urlParam, param, headers) {
        return this.request("POST", path, urlParam, param, headers)
    }
}

//=============================================================
const hc = new HttpClient("http://localhost:8888", false, {timeout: 10000} )
hc.token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2MDI5MTkzMTAsImlzcyI6ImthaXMiLCJsb2dpbiI6dHJ1ZX0.P4ZiqRADhpOZMeVF-y-LXdAUM-virURvsT0FupSjyYg"

let all_count = 0, succeed_count = 0, failed_count = 0

return (async () => {
    hc.setDebug(false)
   
    const ar = []
	const count = process.argv.length < 3 ? 1 : 0 + process.argv[2]

    for (let i = 0; i < count; i++) {
		const req = http.request({
			hostname: "127.0.0.1",
			port: 8888,
			path: "/hello/",
			method: "GET"
		}, res => {
			data = [];
			res.on("data", d => data += d)
			res.on("end", () => {
				console.log("[%d] %s", i, data)
				++all_count
				++succeed_count
				if (all_count == count) {
					console.log("------------------------------")
					console.log("test complete, count = %d, succeed = %d, failed = %d",
						all_count, succeed_count, failed_count)
				}
			})
		})
		req.on("error", e => {
            req.destroy()
			console.log("[" + i + "] ", e)
			++all_count
			++failed_count
			if (all_count == count) {
				console.log("------------------------------")
				console.log("test complete, count = %d, succeed = %d, failed = %d",
					all_count, succeed_count, failed_count)
			}
		})
		req.end()
	}

	/*
	let aar;
	try {
		aar = await Promise.all(ar);
	} catch (e) {
		console.log(e);
	}
	for (let i in aar)
		console.log("%d,%j", i, aar[i]);
	*/
    //const r = await hc.post("/hello/")
    //console.log("result:", r)
	/*
    for (let i = 0; i < 1000; i++)
		console.log(i, "result:", await hc.post("/hello/"));
	*/

})()

