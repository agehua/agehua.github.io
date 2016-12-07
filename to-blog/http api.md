
关于 HTTP REST API 的设计，为今后 project 前后端分离做准备，主要看看有什么地方会发生争议。
 
REST API Design Guide

使用HTTP头提供版本信息，而不是 URI

以下形式为 Bad Practice:

/v1/blah/blahblah/…
/v2/blah/blahblah/…
使用 HTTP 头的 Accept 代替:

Accept: application/vnd.nhn.project+json; version=1
URI只包含名词，不包含动词

 
 
 
单复数形式要看资源(Resource)的关系，如果是对n的关系，使用复数形式；如果是对1的关系，使用单数形式即可。

Bad:

POST /infrastructure/instances/{id}/associate_ip -> 关联 IP
POST /infrastructure/instances/{id}/deassociate_ip -> 解除 IP 关联
Good:

POST /infrastructure/instances/{id}/ip_associations -> “创建” IP 关联
DELETE /infrastructure/instances/{id}/ip_associations/{association_id} -> “删除”(解除) IP 关联
注意这里使用了 DELETE Verb，将在下节中讲到。

正确使用 HTTP Verbs

常用的 HTTP Verbs 有以下几种，具备不同的含义：

Verb	Description
GET	获取单个或多个资源
POST	创建单个资源
PUT	用于修改单个资源（注意与PATCH的区别）
PATCH	一般用不到，只在修改资源中的单个字段时使用
DELETE	删除资源
正确使用 HTTP Status Code

200 OK - Response to a successful GET, PUT, PATCH. Can also be used for a POST that doesn’t result in a creation.
201 Created - Response to a 202 Acceptedthat results in a creation. Should be combined with a Location header pointing to the location of the new resource
202 Accepted - 异步资源
204 No Content - Response to a successful request that won’t be returning a body (like a DELETE request)
206 Partial Content - 分页时候使用
304 Not Modified - Used when HTTP caching headers are in play
400 Bad Request - The request is malformed, such as if the body does not parse
401 Unauthorized - When no or invalid authentication details are provided. Also useful to trigger an auth popup if the API is used from a browser
403 Forbidden - When authentication succeeded but authenticated user doesn’t have access to the resource
404 Not Found - When a non-existent resource is requested
405 Method Not Allowed - When an HTTP method is being requested that isn’t allowed for the authenticated user
410 Gone - Indicates that the resource at this end point is no longer available. Useful as a blanket response for old API versions
415 Unsupported Media Type - If incorrect content type was provided as part of the request
422 Unprocessable Entity - Used for validation errors
429 Too Many Requests - When a request is rejected due to rate limiting
其他类型，比如服务器处理失败，返回 HTTP 500 即可

不要使用 envelop 包裹 Response Body

一般来说，不要像这样包裹 Response Body:

{
    "header": {
        "status": 200,
        "successful": true,
        "message": ""
    }, 
    "data": {
    }
}
 
 
 
因为 HTTP Status Code 完全可以代替他们，对于 HTTP 200 OK 的志愿，检查 header 也毫无意义。

NOTE 只有在 JSONP 的情况下，包裹 Response Body，因为使用 JSONP 不能拿到 HTTP Status Code 等信息。

错误信息

当错误发生时，禁止返回 HTTP 2XX 的 Response，应该根据情况，返回一个 HTTP 错误码，并跟上如下 Response Body:

{
    "status": 400,
    "message": "xxxxxx"
}
 
 
 
如果为验证错误，返回 HTTP 422 的同时，提供具体的错误信息：

{
    "status": 422,
    "message": "xxxxx",
    "errors": [{
       "field": "name",
       "message": "Required"
    }]
}
分页

 
 
 
分页一般有两种形式，根据资源的情况，分别对应不同的分页形式：

传统：因为提供了总个数，当前起始值，可以实现全功能的分页。
游标(Cursor): 这种形式只能支持 前一页/后一页 的功能。
传统

因为我们不需要包裹Response Body，所以返回的分页形式是放在 HTTP 头里的，对于传统分页，我们选择 Range 头（包括 Range, Accept-Ranges 以及 Content-Range），参见： http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35.2

例子：

GET /users
200 OK
Accept-Ranges: x-entity
Content-Range: x-entity 2-4/10

[{"id": 2}, {"id": 3}, {"id": 4}]
 
 
 
如果资源不可计量，可以使用:

Content-Range: x-entity {start}-{end}/*
 
 
 
当客户端请求时，不使用 query string 传递 range，而是使用 Range 头。

例，取出 id 范围在 4~7 的 用户：

GET /users
Range: x-entity=4-7
游标(Cursor)

 
 
 
参见GitHub的Pagination: https://developer.github.com/v3/#pagination

Link 头

分页信息应存放在 Link 头中，例如：

Link: <https://project-api/gamedata/storage?cursor=abc>; rel="prev",
  <https://project-api/gamedata/storage?cursor=def>; rel="next"
 
 
 
其中，rel 可以为：

prev
next
first
last


-------------------------

API Versioning 有依据了:
https://developer.github.com/v3/media/

Accept 头

application/vnd.abcd.project[.version][+json]
 
 
第一版使用此Accept头

application/vnd.abcd.project.v1+json
 
 
如果不指定，或使用以下的Accept头，默认使用最新API:

application/json
application/vnd.abcd.project+json
 

