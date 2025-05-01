<script setup>
import Dropdown from "@/commands/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      create
    </template>
    <h4>Request:</h4>
    <pre v-pre>
      <code>
create &ltVNAME&gt &ltVTYPE&gt
      </code></pre>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
create a u32

create b cf128

create c union { a u32, b enum { foo, bar } }

create d struct { a i32, c union { a u32, b f32 } }

create e [][][] struct { i i32 }

create f [10][5][6] f32
    </code></pre>
    <h4>Response:</h4>
    <pre v-pre><code>
{ "status" : "SUCCESS" }

{ "status" : "&ltERROR STATUS CODE&gt", "message" : "&ltError MESSAGE&gt" }
    </code></pre>
    <br>
    <h6>Error Status Codes:</h6>
    <br>
    ERR_SYNTAX: Syntax error in the create command
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
</template>

<style scoped>


</style>
