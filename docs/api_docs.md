# API Documentation

## Instance route

### POST `/instance`
Create a new game instance

Response:
```json
{
    "ID": "string"
}
```


## Player route

### POST `/player`
Create a new player within a given instance

Request:
```json
{
    "instanceID": "string"
}
```

Response:
```json
{
    "instanceID": "string",
    "ID": "string"
}
```