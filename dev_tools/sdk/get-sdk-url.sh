#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

GAIA_S3_SDK_BUCKET="gaia-sdk/private-releases"

# The default expiration time for presigned URLs is 5 days
EXPIRATION=432000
PROFILE="default"

print_help() {
    echo "Helps creating presigned URLs from existing S3 SDK files"
    echo " -f|--file        The SDK file to create the URL for. [Optional, if not specified the latest file will be used]"
    echo " -c|--customer    The customer that will use this URL"
    echo " -v|--version     The SDK version [eg 0.1.0]"
    echo " -e|--expiration  The expiration for the presigned URL in seconds. [Optional, default value: 5 days]"
    echo " -l|--list        List all the files for the given version/customer"
    echo " -p|--profile     AWS profile name [Optional, default is default]"
    echo ""
    echo "Examples:"
    echo "  Generate the URL for the file gaia-0.1.0_amd64-20210107162449.deb:"
    echo "    get-sdk-url.sh -v 0.1.0 -c customer1 -f gaia-0.1.0_amd64-20210107162449.deb "
    echo ""
    echo "  Generate the URL for the latest file in the customer bucket:"
    echo "    get-sdk-url.sh -v 0.1.0 -c customer1"
    echo ""
    echo "  List all the SDK files for the customer1"
    echo "    get-sdk-url.sh -v 0.1.0 -c customer1 -l"
}

while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
    -f | --file)
        FILE="$2"
        shift # past argument
        shift # past value
        ;;
    -c | --customer)
        CUSTOMER="$2"
        shift # past argument
        shift # past value
        ;;
    -v | --version)
        VERSION="$2"
        shift # past argument
        shift # past value
        ;;
    -e | expiration)
        EXPIRATION="$2"
        shift # past argument
        shift # past value
        ;;
    -l | --list)
        LIST_MODE=true
        shift # past argument
        ;;
    -p | --profile)
        PROFILE="$2"
        shift # past argument
        shift # past value
        ;;
    -h | --help)
        print_help
        exit 0
        ;;
    *)
        echo "Unrecognized option $key"
        print_help
        exit 1
        ;;
    esac
done

if [[ -z $CUSTOMER ]]; then
    echo "The Customer must be specified"
    exit 1
fi

if [[ -z $VERSION ]]; then
    echo "The Version must be specified"
    exit 1
fi

# Build the final bucket. Eg s3://gaia-sdk/private-releases/0.1.0/customer1
SDK_S3_BUCKET_URI="s3://$GAIA_S3_SDK_BUCKET/$VERSION/$CUSTOMER"

if [ "$LIST_MODE" = true ]; then
    echo "Listing all the files in $SDK_S3_BUCKET_URI"
    aws s3 ls "$SDK_S3_BUCKET_URI/" --profile "$PROFILE" --region "us-west-2"
    exit 0
fi

if [ -z "$FILE" ]; then
    FILE=$(aws s3 ls "$SDK_S3_BUCKET_URI/" --profile "$PROFILE" --region "us-west-2" | tail -n 1 | awk '{print $4}')
fi

echo "Generating presigned URL for $FILE"
PRESIGNED_URL=$(aws s3 presign "$SDK_S3_BUCKET_URI/$FILE" --expires-in "$EXPIRATION" --profile "$PROFILE" --region "us-west-2")
echo ""
echo "The presigned URL is: $PRESIGNED_URL"
