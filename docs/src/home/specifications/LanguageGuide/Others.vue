<script setup>

import Primitive from "@/home/specifications/LanguageGuide/types/primitive.vue";
import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      Errors
    </template>
    <br>
    <h6>Error Status Codes:</h6>
    <br>
    ERR_SYNTAX: Syntax error in the delete command
    <br>
    ERR_TRANSACTION: Something went wrong during the transaction
    <br>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
{ "status" : "SUCCESS" }
{ "status" : "ERR_SYNTAX", "message" : "Invalid token 'c' in column 5" }
{ "status" : "ERR_TRANSACTION", "message" : "Power outage error" }
    </code></pre>
  </Dropdown>
  <Dropdown>
    <template #title>
      Range String
    </template>
    <br>
    <h6>Range Strings</h6>
    <pre v-pre><code>
[INDEX] | [START[:STOP[:STEP]]]
    </code></pre>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
[0]
[0:1]
[0:1:2]
[-1]
[0:-1:]
[::]
[:9:2]
    </code></pre>
  </Dropdown>

</template>

<style scoped>

</style>