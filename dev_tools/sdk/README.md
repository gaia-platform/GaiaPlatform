The scripts in this folder allow uploading the SDK artifacts to S3. At the time of writing (01/2021) we assume that the 
SDK is released to a small circle of initial customers. For this reason we want to have tight control over who can
access the S3 resources and for how much time. 

To achieve such control we use s3 presigned URLs: 
https://docs.aws.amazon.com/AmazonS3/latest/dev/ShareObjectPreSignedURL.html. The presigned URLs are valid only for the 
specified duration. Anyone who receives the presigned URL can then access the object.

The script `publish-sdk-to-s3.sh` uploads SDK files to S3 and generates a presigned URL. The artifacts are published 
in subfolders specific to version and customer. Eg: `s3://gaia-sdk/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210107162449.deb`

The script `get-sdk-url.sh` can generate presigned URLs for existing S3 objects.

Please look at the scripts `--help` output for more details on their usage.

# AWS SDK

The scripts use the AWS CLI V2, follow these instruction to install it: 
https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html

# AWS Credentials

To operate the scripts you need to use AWS credentials that have permissions for S3. The credentials should
be stored in `~/.aws/credentials` (`aws configure`). The `sdk` IAM user has the right credentials to access the bucket.

# TeamCity

The scripts are installed in the TeamCity server in `/home/seagate2/~/.local/bin/` which is part of `$PATH`. The right
AWS credentials are installed on the machine.

## Publish the SDK

Let say you want to publish the SDK generated as part of the TeamCity build 10893:
`/opt/TeamCity/.BuildServer/system/artifacts/GaiaPlatform/ProductionGaiaRelease_gdev/10893/gaia-0.1.0_amd64.deb`

You can run, from anywhere:
```bash
$ publish-sdk-to-s3.sh -v 0.1.0 -c customer1 \
                     -f /opt/TeamCity/.BuildServer/system/artifacts/GaiaPlatform/ProductionGaiaRelease_gdev/10893/gaia-0.1.0_amd64.deb
```

The output is something like this:

`The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108081334.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T161344Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY`

You can give this URL to the customer `customer1`, they will have 5 days to use it before expiration.

## Generate the URL for existing SDK

Let say you need to retrieve the URL for an SDK that has already been uploaded. You can list all the existing SDK
for a given customer:

```
$ get-sdk-url.sh -v 0.1.0 -c customer1 -l
Listing all the files in s3://gaia-sdk/private-releases/0.1.0/customer1
2021-01-08 08:13:36   15096308 gaia-0.1.0_amd64-20210108081334.deb
2021-01-08 08:19:04   15096308 gaia-0.1.0_amd64-20210108081902.deb
2021-01-08 08:20:55   15096308 gaia-0.1.0_amd64-20210108082053.deb
```

You can now create a new URL using one of the files listed by the previous command:

```
$ get-sdk-url.sh -v 0.1.0 -c customer1 -f gaia-0.1.0_amd64-20210108082053.deb
Generating presigned URL for gaia-0.1.0_amd64-20210108082053.deb

The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108082053.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXXXX%XXXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T162330Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY
```

Alternatively you can create an URL for the latest uploaded SDK. Note that the S3 list command sort the output 
alphabetically not by date. Our naming pattern should ensure that the last file in alphabetical order is also the last 
file in chronological order.

```
$ get-sdk-url.sh -v 0.1.0 -c customer1
Generating presigned URL for gaia-0.1.0_amd64-20210108082053.deb

The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108082053.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXXXX%XXXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T162330Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY
```
