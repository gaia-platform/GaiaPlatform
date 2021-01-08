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
