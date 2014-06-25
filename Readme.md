# mkbundle – Create RFC 5050 bundles

This tool doesn't create [bundles](http://tools.ietf.org/html/rfc5050) itself:
it creates the blocks that make up a bundle. These blocks can then be `cat`'d
together, along with payloads, to create a full bundle. The block creation is
done in two phases: generating a JSON parameter file and compiling that
parameter file into a final binary form.

The parameter file contains all the fields that make up a block – flags, EIDs,
etc. – and their values. It can be processed with a tool like
[`json(1)`](http://trentm.com/json/) to mangle the block, perform arithmetic, or
do other advanced scripting.

Currently the two main blocks – primary and extension (everything else) blocks
– can be created using the `primary` and `extension` commands, respectively.
The JSON parameters can then be passed into the `compile` command to generate
the binary form of each block.

# Example

The following script creates a bundle with 4 blocks and some simple payloads:

```sh
(
./mkbundle primary --prio normal --flag singleton --dest ipn:1.2 --src ipn:1.1 \
    --report-to ipn:1.1 --custodian ipn: | ./mkbundle compile

./mkbundle extension --type phib --flag discard-block --payload-length 8 | \
    ./mkbundle compile
printf 'ipn:1.0\0'

./mkbundle extension --type 20 --flag replicate --payload-length 1 | \
    ./mkbundle compile
printf '\0'

./mkbundle extension --type payload --flag replicate --flag last-block \
    --payload-length 5 | ./mkbundle compile
printf 'test\0'
) >test.bundle
```
