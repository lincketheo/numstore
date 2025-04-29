<script setup>
import CommandDropdown from "@/commands/CommandDropdown.vue";
</script>

<template>
  <CommandDropdown>
    <template #title>
      bcreate
    </template>
    <h4>Request:</h4>
    <pre v-pre>
      <code>
bcreate &ltVNAME&gt &ltVTYPE&gt
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
[&ltRETURN BYTE CODE&gt (1 byte)]
    </code></pre>
    <br>
    <h6>Return Byte Codes:</h6>
    <br>
    0: Success
    <br>
    1: Transaction Error
    <br>
    2: Syntax Error
    <br>
  </CommandDropdown>
</template>
