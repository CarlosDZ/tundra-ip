## tundra-ip
tundra-ip follows its own syntaxis, that is the one that will appear on the documentation as the canonical way of using the package.

Aditionally, it is compatible with common ip syntax that external packages expect. While you can use that sintax manually, the point of this is being able to easily set up a symlink to avoid having both tundra-ip and ip.
This compatibility syntax will be documented on its own section "Compatibility with ip-dependant packages".

### Sintax Table
tundra-ip
├── status
├── addr
│   ├── show
│   ├── add <IP>/<prefijo> on <if>
│   ├── del <IP>/<prefijo> on <if>
│   └── flush on <if>
├── route
│   ├── show
│   ├── add <red>/<prefijo> on <if>
│   ├── del <red>/<prefijo>
│   ├── default
│   │   ├── show
│   │   ├── add <gw> on <if> metric <n>
│   │   └── del <if>
│   └── flush
└── link
    ├── show
    ├── up <if>
    ├── down <if>
    └── set mac <mac> on <if>

### Flags
--version
--help

### Compatibility with ip-dependant packages
tundra-ip
├── route
│   └── add default via <gw> dev <if>
└── link
    └── set
        ├── up <if>
        ├── down <if>
        └── <if> address <mac>
