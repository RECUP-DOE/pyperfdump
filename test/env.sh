#! /usr/bin/env bash

###
# In order for a test to succeed, we need at least 1 valid counter
# To avoid arbitrarily selecting a counter that doesn't work,
# we will just list all potential counters as "selected" counters
# If no counters are found, or none of these work, then PAPI doesn't work
###

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
