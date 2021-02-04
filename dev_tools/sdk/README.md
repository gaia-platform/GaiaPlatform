The scripts in this folder allow you to generate custom SDK builds that you can target to specific customers. The scripts upload the SDK artifacts to Gaia's Amazon Simple Storage Service (S3) account.

At the time of writing (01/2021) we assume that the SDK is released to a small circle of initial customers. Use these scripts to generate presigned URLs that are valid only for the specified duration. Anyone who receives the presigned URL can then access the object. This allows us to keep tight control over who can access the S3 resources and for how much time.

For more information about presigned URLs, see [Share an object with others](https://docs.aws.amazon.com/AmazonS3/latest/dev/ShareObjectPreSignedURL.html) in the AWS documentation.  

The script `publish-sdk-to-s3.sh` uploads SDK files to the S3 bucket and generates a presigned URL. The artifacts are published 
in subfolders that are organized by version and then by the customers that have received that version. For example:  

`s3://gaia-sdk/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210107162449.deb`

Use the `get-sdk-url.sh` script to list existing S3 objects and generate presigned URLs for those objects.

For details about the usage of these scripts, see the `--help` output. 

# AWS SDK

The scripts use the AWS CLI V2. For installation instructions, see: 
https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html

# AWS Credentials

To run the scripts, you must use AWS credentials that have permissions for S3. Store the credentials in `~/.aws/credentials` (`aws configure`). The `sdk` IAM user has the correct credentials to access the S3 bucket.

# TeamCity

While the scripts are agnostic as to where the artifacts are, it is a best practice to run them on the machine where TeamCity runs.

The scripts are installed in the TeamCity server in `/home/seagate2/~/.local/bin/` which is part of `$PATH`. The right
AWS credentials are installed on the machine.

## Publish the SDK

To publish a targeted SDK and upload it to the S3 bucket use the `publish-sdk-to-s3.sh` script.

### Example

Let say you want to publish the SDK generated as part of the TeamCity build 10893:
`/opt/TeamCity/.BuildServer/system/artifacts/GaiaPlatform/ProductionGaiaRelease_gdev/10893/gaia-0.1.0_amd64.deb`

You can run, from anywhere:
```bash
$ publish-sdk-to-s3.sh -v 0.1.0 -c customer1 \
                     -f /opt/TeamCity/.BuildServer/system/artifacts/GaiaPlatform/ProductionGaiaRelease_gdev/10893/gaia-0.1.0_amd64.deb
```

The output will look similar to the following:

`The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108081334.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T161344Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY`

You can give this URL to the customer `customer1`. They will have 5 days to use it before expiration.

## Generate the URL for existing SDK

To publish a targeted SDK for an SDK that has already been uploaded to the S3 Bucket use the 'get-sdk-url.sh' script.

### Example
Let's say that you need to retrieve the URL for an SDK that has already been uploaded to the S3 Bucket. You can list all the existing SDK
for a given customer:

```
$ get-sdk-url.sh -v 0.1.0 -c customer1 -l
Listing all the files in s3://gaia-sdk/private-releases/0.1.0/customer1
2021-01-08 08:13:36   15096308 gaia-0.1.0_amd64-20210108081334.deb
2021-01-08 08:19:04   15096308 gaia-0.1.0_amd64-20210108081902.deb
2021-01-08 08:20:55   15096308 gaia-0.1.0_amd64-20210108082053.deb
```

The following example shows how to create a URL for the gaia-0.1.0_amd64-20210108082053.deb version of the SDK:

```
$ get-sdk-url.sh -v 0.1.0 -c customer1 -f gaia-0.1.0_amd64-20210108082053.deb
Generating presigned URL for gaia-0.1.0_amd64-20210108082053.deb

The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108082053.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXXXX%XXXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T162330Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY
```

To create a URL for the latest uploaded SDK:

```
$ get-sdk-url.sh -v 0.1.0 -c customer1
Generating presigned URL for gaia-0.1.0_amd64-20210108082053.deb

The presigned URL is: https://gaia-sdk.s3.us-west-2.amazonaws.com/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210108082053.deb?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=XXXXX%XXXX%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20210108T162330Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=YYYY
```

**Important**:

The S3 list command sorts the output alphabetically, not by date. Our naming pattern should ensure that the last file in alphabetical order is also the last file in chronological order.
