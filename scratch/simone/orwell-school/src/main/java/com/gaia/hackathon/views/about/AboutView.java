package com.gaia.hackathon.views.about;

import com.vaadin.flow.component.Text;
import com.vaadin.flow.component.html.Div;
import com.vaadin.flow.router.PageTitle;
import com.vaadin.flow.router.Route;
import com.gaia.hackathon.views.main.MasterDetailLayout;

@Route(value = "about", layout = MasterDetailLayout.class)
@PageTitle("About")
public class AboutView extends Div {

    public AboutView() {
        setId("about-view");
        add(new Text("Mind your business!!!!"));
    }

}
