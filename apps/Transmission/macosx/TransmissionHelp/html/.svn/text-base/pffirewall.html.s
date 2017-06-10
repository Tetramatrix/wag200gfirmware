<html>

	<head>
		<META http-equiv="Content-Type" content="text/html; charset=utf-8" />
		<link media="all" rel="stylesheet" href="../styles/TransBody.css" />
		<title>Port Forwarding the Mac OS X Firewall</title>
	</head>
	<body>
		<div id="mainbox">
			<div id="banner">
				<a name="menus"></a>
				<div id="machelp">
				<a class="bread" href="../index.html">Transmission Help</a>
				</div>
				<div id="index">
				<a class="leftborder" href="../html/Index2.html">Index</a></div>
				</div>
			</div>
			<div id="pagetitle">
				<h1>Port Forwarding the Mac OS X Firewall</h1>
			</div>
				<br>
				<h3>On 10.4 Tiger</h3>
				<div summary="To do this" id="taskbox">
					<ol>
						<li>Open Transmission, go to Preferences >> Network and enter a number for the port. It is recommended you pick a random number between 49152 and 65535. Let's use 50001 for now. Then quit Transmission. </li>
						<li>Open System Prefs >> Sharing >> Firewall. Click "New." In the "Port Name" pop-up menu, select Other, and fill in the settings as follows: </li>
							<ul>
								<li>TCP Port Number(s): the port you chose in step 1 - eg 50001. </li>
								<li>UDP Port Number(s): the port you chose in step 1 - eg 50001. </li>
								<li>Description: Transmission </li>
							</ul>
						<li>Click OK. </li>
						<p>NB: To disable the firewall, click Stop.
					</ol>
				</div>
				<h3>On 10.5 Leopard</h3>
				<p>Upon opening Transmission for the first time, a Mac OS X dialogue box should appear asking if you will allow Transmission to receive incoming connections. Click Accept.
				<p>If this doesn't happen, you can add Transmission to Leopard's firewall manually:
					<div summary="To do this" id="taskbox">
						<ol>
							<li>Open System Prefs >> Security >> Firewall. Make sure "Set access for specific services and applications" is selected.</li>
							<li>Click the "+" button and select Transmission from you Applications folder. </li>
							<li>Make sure the pull down menu is set to "Allow incoming connections".
						</ol>
					</div>
		</div>
	</body>
	
</html>