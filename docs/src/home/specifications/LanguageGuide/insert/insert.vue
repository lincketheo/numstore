<script setup>
import Dropdown from "@/home/specifications/LanguageGuide/DropDown.vue";
</script>

<template>
  <Dropdown>
    <template #title>
      insert
    </template>
    <h4>Request:</h4>
    <br>
    <em>Note: when &ltINDEX&gt is equal to -1, insert is the same as append</em>
    <br>
    <br>
    <h6>Insert a single element to a single variable:</h6>
    <pre v-pre><code>insert &ltINDEX&gt { "VNAME" : &ltDATA ELEMENT&gt }</code></pre>
    <br>
    <h6>Insert a list of elements to a single variable:</h6>
    <pre v-pre><code>insert &ltINDEX&gt { "VNAME" : [&ltDATA ELEMENT&gt (, &ltDATA ELEMENT&gt)*] }</code></pre>
    <br>
    <h6>Insert and link multiple variables together (1 of each):</h6>
    <pre v-pre><code>insert &ltINDEX&gt {
      "VNAME1" : &ltDATA ELEMENT 1&gt,
      "VNAME2" : &ltDATA ELEMENT 2&gt
}</code></pre>
    <br>
    <h6>Insert and link multiple variables together (multiple of each):</h6>
    <pre v-pre><code>insert &ltINDEX&gt {
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

insert 10 { "a" : 42 }
insert 9 { "b" : { "x" : 1.0, "y" : 2.0 } }
insert 20 { "c" : { "f" : 3.14 } }
insert 9 { "d" : [1, 2, 3, 4] }
insert 2 { "e" : [1, 0, 1] }
insert 1 { "f" : "GREEN" }
insert 9 { "g" : { "id" : 123, "history" : [0.1, 0.2, 0.3] } }
insert 5 { "h" : [[1,2], [3,4]] }

insert 10 {
  "a" : 5,
  "f" : "RED"
}

insert -1 {
  "d" : [5, 6, 7],
  "e" : [0, 1, 0]
}

insert 1 {
  "b" : [ { "x" : 1.1, "y" : 2.2 }, { "x" : 3.3, "y" : 4.4 } ],
  "c" : [ { "i" : 7 }, { "f" : 6.28 } ]
}

insert 0 {
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
