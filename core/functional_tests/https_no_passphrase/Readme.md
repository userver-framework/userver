Certificates and private keays were generated via:

```
openssl genrsa 4096 > private_key.key
openssl req -x509 -sha256 -nodes -new -key private_key.key -out cert.crt
```

Use more advanced techniques for production - properly sign with root
certificate and so forth...
