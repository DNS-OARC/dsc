<sisy:style id="common"
	signature="sourceforge">

	body {
		background-color: white;
		color: black;
	}

	/* tags in NS do not inherit body attributes */
	body, td {
		font-family: Arial, Helvetica, sans-serif;
	}

	div.page-signature {
		float: right;
	}

	div.buttons {
		clear: both;
	}

	div.buttons-left {
		float: left;
		width: 80%;
	}

	div.buttons-right {
		float: right;
	}

	span.button, span.button-spacer {
		font-weight: bold;
		background-color: transparent;
		color: gray;
	}

	span.button {
		text-transform: uppercase;
		text-decoration: none;
		white-space: nowrap;
	}

	span.button a {
		text-decoration: none;
		background-color: transparent;
		color: #194096;
	}

	span.button a:hover {
		text-decoration: underline;
		background-color: transparent;
		color: #671305;
	}

	h1 {
		text-transform: uppercase;
		margin-top: 3ex;
		margin-bottom: 3ex;
		background-color: transparent;
		color: #194096;
	}

	h2 {
		font-weight: bold;
		background-color: transparent;
		color: #671305;
	}

	h3 {
		font-weight: normal;
		background-color: transparent;
		color: #671305;
	}

	div.ex {
		border: solid thin;
		width: 85%;
		margin-left: auto;
		margin-right: auto;
		background-color: #EAF1F7;
		color: black;
	}

	div.ex div.heading {
		padding: 1em;
		background-color: #DBE6DE;
		color: #671305;
	}

	div.ex .tag {
		background-color: transparent;
		color: #671305;
		font-variant: small-caps;
		font-weight: bold;
	}

	div.ex div.main {
		width: auto;
		padding-left: 1em;
	}

	div.ex pre.main-body {
		margin-bottom: 0;
		padding-left: 1ex;
		font-size: 12pt;
	}

	div.ex p.source {
		margin-top: 0;
		background-color: transparent;
		color: white; /* #671305; */
		float: right;
		margin-bottom: 0;
	}

	div.ex p.source a:link {
		text-decoration: none;
	}

	div.ex div.trailer {
		clear: both;
		padding: 1em;
		background-color: #DDE4F2; /* #671305; #194096; */
		color: black;
	}

	div.sect {
		margin-left: 2em;
	}

	span.concept {
		font-weight: bold;
	}

	span.programname, span.filename {
		font-style: italic;
	}

	span.codesample {
		white-space: nowrap;
		font-style: italic;
	}

	pre.codesample {
		background-color: #EAF1F7;
		color: black;
	}

	td.profile {
		font-style: italic;
	}

	th {
		text-align: left;
	}

	tr.heading {
		background-color: #DBE6DE;
		color: black;
	}

	tr.even {
		background-color: #EAF1F7;
		color: black;
	}

	tr.odd {
		background-color: #DDE4F2;
		color: black;
	}

	td.number {
		text-align: right;
	}

	table.syntax td.pree pre {
		margin-left: auto;
		margin-right: auto;
	}

	table.builtin td.impl {
		text-align: center;
		white-space: pre;
	}

	div.primer li a {
		text-decoration: none;
	}

</sisy:style>

<sisy:style id="L1"
	header="hapy"
	active_buttons="home,docs,releases,support">

	<sisy:import style="common" />

</sisy:style>

<sisy:style id="docs"
	header="hapy"
	active_buttons="home,docs,primer,examples,prefix,syntax,builtins,parser,pree,actions,debug,support">

	<sisy:import style="common" />

</sisy:style>



<sisy:header id="hapy"
	alt=""
	link=""
	bgcolor="#FFFFFF"
/>

<sisy:button id="home"
	alt="Home"
	link="index.html"
/>

<sisy:button id="primer"
	alt="Primer"
	link="primer.html"
/>

<sisy:button id="examples"
	alt="Examples"
	link="examples.html"
/>

<sisy:button id="support"
	alt="Support"
	link="support.html"
	align="right"
/>

<sisy:button id="releases"
	alt="Releases"
	link="releases.html"
/>

<sisy:button id="spacer"
	alt=" &middot; "
/>

<sisy:button id="sourceforge"
	alt="SourceForge Home"
	src="http://sourceforge.net/sflogo.php?group_id=94942&amp;type=1"
	width="88"
	height="31"
	link="http://sourceforge.net/"
/>

<sisy:button id="syntax"
	alt="Syntax"
	link="syntax.html"
/>

<sisy:button id="builtins"
	alt="Rules"
	link="builtins.html"
/>

<sisy:button id="pree"
	alt="Tree"
	link="pree.html"
/>

<sisy:button id="parser"
	alt="Parser"
	link="parser.html"
/>

<sisy:button id="prefix"
	alt="Prefixing"
	link="prefix.html"
/>

<sisy:button id="actions"
	alt="Actions"
	link="actions.html"
/>

<sisy:button id="debug"
	alt="Debug"
	link="debug.html"
/>

<sisy:button id="docs"
	alt="Docs"
	link="docs.html"
/>

