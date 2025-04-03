## S3 API

**Quality:** @ref QUALITY_TIERS "Silver Tier".

## Introduction

🐙 **userver** provides client to any endpoint that implements client to any S3
   endpoints. It uses ```namespace s3api```

The client is currently not feature-complete and exposes only some of the possible
methods, parameters and settings. The client also suffers from some
weird design decisions, like explicit selection of "http://" vs "https://". We
intend to gradually improve its quality in consequent releases.

See also:
* @ref scripts/docs/en/userver/tutorial/s3api.md
* [Official S3 API](https://docs.aws.amazon.com/AmazonS3/latest/API/Type_API_Reference.html)

## Usage and testing

* @ref scripts/docs/en/userver/tutorial/s3api.md shows hot to create, use and test
  client

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/easy.md | @ref scripts/docs/en/userver/libraries/grpc-reflection.md ⇨
@htmlonly </div> @endhtmlonly
