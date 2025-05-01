<script setup>
import Dropdown from "@/commands/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      append
    </template>
    <h4>Request:</h4>
    <br>
    <h6>Append a single element to a single variable:</h6>
    <pre v-pre><code>append { "VNAME" : &ltDATA ELEMENT&gt }</code></pre>
    <br>
    <h6>Append a list of elements to a single variable:</h6>
    <pre v-pre><code>append { "VNAME" : [&ltDATA ELEMENT&gt (, &ltDATA ELEMENT&gt)*] }</code></pre>
    <br>
    <h6>Append and link multiple variables together (1 of each):</h6>
    <pre v-pre><code>append {
      "VNAME1" : &ltDATA ELEMENT 1&gt,
      "VNAME2" : &ltDATA ELEMENT 2&gt
}</code></pre>
    <br>
    <h6>Append and link multiple variables together (multiple of each):</h6>
    <pre v-pre><code>append {
      "VNAME1" : [&ltDATA ELEMENT 1&gt (,&ltDATA ELEMENT 1&gt)*],
      "VNAME2" : [&ltDATA ELEMENT 2&gt (,&ltDATA ELEMENT 2&gt)*]
}</code></pre>
    <br>
    <h6>Examples</h6>
    <pre v-pre><code>
create a u32
create b struct { x f32, y f32 }
create c union { i i32, f f64 }
create d []i8
create e [3]bool
create f enum { RED, GREEN, BLUE }
create g struct { id u64, history []f32 }
create h [][]u16

append { "a" : 42 }
append { "b" : { "x" : 1.0, "y" : 2.0 } }
append { "c" : { "f" : 3.14 } }
append { "d" : [1, 2, 3, 4] }
append { "e" : [1, 0, 1] }
append { "f" : "GREEN" }
append { "g" : { "id" : 123, "history" : [0.1, 0.2, 0.3] } }
append { "h" : [[1,2], [3,4]] }

append {
  "a" : 5,
  "f" : "RED"
}

append {
  "d" : [5, 6, 7],
  "e" : [0, 1, 0]
}

append {
  "b" : [ { "x" : 1.1, "y" : 2.2 }, { "x" : 3.3, "y" : 4.4 } ],
  "c" : [ { "i" : 7 }, { "f" : 6.28 } ]
}

append {
  "g" : [
    { "id" : 100, "history" : [0.5, 0.6] },
    { "id" : 101, "history" : [0.7, 0.8] }
  ],
  "h" : [
    [[5,6],[7,8]],
    [[9,10],[11,12]]
  ]
}
</code></pre>
  </Dropdown>
</template>
