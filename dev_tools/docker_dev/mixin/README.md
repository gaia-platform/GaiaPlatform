# Gdev Mixins
Gdev builds minimal size images for each target. To keep images as lean as possible, we do not
include any utilites such as gdb or sudo in the images. Such utilities should be provided by mixins
defined in this directory.

Note that mixins are named after their directory name. Given a target such as `demos/incubator`,
our basic build image is named `demos__incubator:<git hash>`. After applying a mixin such as `base`,
we produce an image named `base__demos_incubator:<git hash>`. More generally, the mixed image name
name `<mixin name>__<image_name>:<git hash>`.

Mixins allow some flexibility in applying different sets of configuration on top of basic build
images, but without creating a combinatorial explosion for our automated build systems (we don't
have to build each type of specialized image since specialization is applied on-demand from the
mixin).
