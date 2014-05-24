{
	"service": {
		"api": "http",
			"port": 8000
	},
	"http": {
		"script_names": ["lol"],
		"timeout": 3600,
	},
	"security": {
		"csrf" : {
			"enable" : true
		}
	},
	"session": {
		"location": "client",
		"expire": "renew",
		"cookies": {
			"prefix": "cses",
		},
		"client": {
			"encryptor": "aes",
			"key": "b2908835d5584267aa599d4f85138a2b",
		},
	},
	"logging": {
		"level": "debug",
	},
	"localization": {
		"locales": ["en_US.UTF-8"],
	}
}
