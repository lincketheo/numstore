<script setup>
import Dropdown from "@/commands/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      update
    </template>
    <h4>Request:</h4>
    <br>
    <h6>Update a single element to a single variable:</h6>
    <pre v-pre><code>update &ltINDEX&gt { "VNAME" : &ltDATA ELEMENT&gt }</code></pre>
    <br>
    <h6>Update a list of elements to a single variable:</h6>
    <br>
    <em>Note &ltRANGE_STR&gt must match the shape of the data supplied</em>
    <br>
    <pre v-pre><code>update &ltRANGE_STR&gt { "VNAME" : [&ltDATA ELEMENT&gt (, &ltDATA ELEMENT&gt)*] }</code></pre>
    <br>
    <h6>Update and link multiple variables together (1 of each):</h6>
    <pre v-pre><code>update &ltINDEX&gt {
      "VNAME1" : &ltDATA ELEMENT 1&gt,
      "VNAME2" : &ltDATA ELEMENT 2&gt
}</code></pre>
    <br>
    <h6>Update and link multiple variables together (multiple of each):</h6>
    <pre v-pre><code>update &ltRANGE_STR&gt {
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

update 10 { "a" : 42 }
update 9 { "b" : { "x" : 1.0, "y" : 2.0 } }
update 20 { "c" : { "f" : 3.14 } }
update 9 { "d" : [1, 2, 3, 4] }
update 2 { "e" : [1, 0, 1] }
update 1 { "f" : "GREEN" }
update 9 { "g" : { "id" : 123, "history" : [0.1, 0.2, 0.3] } }
update 5 { "h" : [[1,2], [3,4]] }

update 10 {
  "a" : 5,
  "f" : "RED"
}

update -1 {
  "d" : [5, 6, 7],
  "e" : [0, 1, 0]
}

update [1:3:3] {
  "b" : [ { "x" : 1.1, "y" : 2.2 }, { "x" : 3.3, "y" : 4.4 } ],
  "c" : [ { "i" : 7 }, { "f" : 6.28 } ]
}

update [1::3] {
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
