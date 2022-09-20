public int main (string[] args) {
	// Output: ``s[ai]mple matched!``
	try {
		string quote = """
Reading package lists... Done
Building dependency tree       
Reading state information... Done
Some packages could not be installed. This may mean that you have
requested an impossible situation or if you are using the unstable
distribution that some required packages have not yet been created
or been moved out of Incoming.
The following information may help to resolve the situation:

The following packages have unmet dependencies:
 command line argument : Depends: qtdeclarative5-controls-plugin but it is not installable
                         Depends: gstreamer0.10-plugins-ugly but it is not installable
E: Unable to correct problems, you have held broken packages.
		""";
		var regex = new Regex (" Depends: (.*) but it is not installable");
		MatchInfo match_info;
		if (regex.match (quote, 0, out match_info)) {
			string []missings = {};
			while (match_info.matches ()) {
				var word = match_info.fetch (1);// 1类似\1，取匹配的括号项, 取全部文本时赋0
				missings += word;
				match_info.next ();
				print ("match:%s\n", word);
			}
		}
		
	} catch (RegexError e) {
		print ("Error %s\n", e.message);
	}
	return 0;
}

