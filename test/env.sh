#! /usr/bin/env bash

PAPI_EVENTS="$(papi_avail | grep -E "Yes[ ]+(No|Yes)" | awk '{print $1}')"
for event in $PAPI_EVENTS ; do
  [ -n "$PDUMP_EVENTS" ] && PDUMP_EVENTS=",$PDUMP_EVENTS"
  PDUMP_EVENTS="$event$PDUMP_EVENTS"
done
PAPI_EVENTS="$(papi_native_avail | grep "::" | awk '{print $2}')"
for event in $PAPI_EVENTS ; do
  [ -n "$PDUMP_EVENTS" ] && PDUMP_EVENTS=",$PDUMP_EVENTS"
  PDUMP_EVENTS="$event$PDUMP_EVENTS"
done
[ -z "$PDUMP_EVENTS" ] && echo "No PAPI counters found :(" && exit 1
PDUMP_DUMP_DIR="$(pwd)"
export PDUMP_EVENTS
export PDUMP_DUMP_DIR
export PDUMP_OUTPUT_FORMAT=hdf5
