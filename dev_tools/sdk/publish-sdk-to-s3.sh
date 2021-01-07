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
    echo "Uploads the SDK files to S3. The files are stored into gaia-sdk/private-releases folder."
    echo "Each version/customer has its own folder. A typical URL look like:"
    echo " s3://gaia-sdk/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210107162449.deb"
    echo ""
    echo " -f|--file        The SDK file to upload to S3"
    echo " -c|--customer    The customer that will use this URL"
    echo " -v|--version     The SDK version [eg 0.1.0]"
    echo " -e|--expiration  The expiration for the presigned URL in seconds. [Optional, default value: 5 days]"
    echo " -p|--profile     AWS profile name [Optional, default is default]"
    echo ""
    echo "Examples"
    echo "  Publish to S3 the SDK for the customer customer1. This will create an s3 resource: s3://gaia-sdk/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210107162449.deb"
    echo "    ./publish-sdk-to-s3.sh -f cmake-build/gaia-0.1.0_amd64.deb -v 0.1.0 -c customer1 "

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
    -e | --expiration)
        EXPIRATION="$2"
        shift # past argument
        shift # past value
        ;;
    -p | --profile)
        PROFILE="$2"
        shift # past argument
        shift # past value
        ;;
    *)
        echo "Unrecognized option $key"
        exit 1
    esac
done

if [[ ! -f $FILE ]]; then
    echo "The SDK file must be specified and must be a valid file"
    exit 1
fi

if [[ -z $CUSTOMER ]];
then
    echo "The Customer must be specified"
    exit 1
fi

if [[ -z $VERSION ]];
then
    echo "The Version must be specified"
    exit 1
fi

# Append the current date to the sdk filename. This allow different minor versions of the SDK to coexist.
# For instance gaia-0.1.0_amd64.deb -> gaia-0.1.0_amd64-20210107162109.deb
NOW=$(date +"%Y%m%d%H%M%S")
SDK_FILENAME=$(basename "$FILE")
EXTENSION=${SDK_FILENAME##*.}
SDK_FILENAME=$(echo "$SDK_FILENAME" | rev | cut -f 2- -d '.' | rev)
SDK_FILENAME="$SDK_FILENAME-$NOW.$EXTENSION"

# Normalize the customer name to lower case
CUSTOMER=$(echo "$CUSTOMER" | awk '{print tolower($0)}')

# Build the final S3 URL. Eg s3://gaia-sdk/private-releases/0.1.0/customer1/gaia-0.1.0_amd64-20210107162449.deb
SDK_S3_URI="s3://$GAIA_S3_SDK_BUCKET/$VERSION/$CUSTOMER/$SDK_FILENAME"

# Upload the SDK to S3
aws s3 cp "$FILE" "$SDK_S3_URI" --profile "$PROFILE" --region "us-west-2"

# Create the presigned URL
PRESIGNED_URL=$(aws s3 presign "$SDK_S3_URI" --expires-in "$EXPIRATION" --profile "$PROFILE" --region "us-west-2")
echo "The presigned URL is: $PRESIGNED_URL"
