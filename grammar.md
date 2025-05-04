program -> query* EOF

query -> CREATE | APPEND 

CREATE -> create IDENT TYPE 
APPEND -> append IDENT 

TYPE -> ...
