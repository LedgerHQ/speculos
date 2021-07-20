## Generate

Convert the specification file from YAML to JSON:

```shell
docker pull docker.io/swaggerapi/swagger-converter:v1.0.2
docker run --rm -it -p 8080:8080 docker.io/swaggerapi/swagger-converter:v1.0.2
wget --quiet -O- --header 'content-type: application/yaml' --post-file api/swagger.yaml \
    http://127.0.0.1:8080/api/convert | jq . > api/static/swagger/swagger.json
```

The following command can be used to serve (and retrieve) the swagger-ui using the generated JSON file:

```shell
docker run --rm -p 8080:8080 -e SWAGGER_JSON=/api/swagger/swagger.json -v "$(pwd)/speculos/api:/api" docker.io/swaggerapi/swagger-ui
for f in index.html swagger-ui.css favicon-32x32.png favicon-16x16.png swagger-ui-bundle.js swagger-ui-standalone-preset.js; do
    wget --quiet -O "speculos/api/static/swagger/$f" "http://127.0.0.1:8080/$f"
done
```

## References

- https://editor.swagger.io/
- https://swagger.io/specification/
- https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.1.0.md
- https://openapi-generator.tech/docs/online/
