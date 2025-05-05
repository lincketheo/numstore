import  "./assets/main.css"
import { createApp } from 'vue'
import App from './App.vue'
import HomeView from "@/HomeView.vue";
import AboutView from "@/AboutView.vue";
import NewsView from "@/NewsView.vue";

import OverviewAbout from "@/home/overview/About.vue";
import QuickStart from "@/home/overview/QuickStart.vue";

import {createMemoryHistory, createRouter} from "vue-router";
import LanguageGuide from "@/home/specifications/LanguageGuide.vue";
import Configuring from "@/home/specifications/Configuring.vue";
import Changelog from "@/home/development/Changelog.vue";
import Contributing from "@/home/development/Contributing.vue";

const routes = [
    {path: '/', component: HomeView, children: [
            // Default
            {
                path: '',
                component: OverviewAbout,
            },

            // Overview
            {
                path: 'overviewAbout',
                component: OverviewAbout,
            },
            {
                path: 'overviewQuickStart',
                component: QuickStart,
            },

            // Specifications
            {
                path: 'specificationsLanguageGuide',
                component: LanguageGuide,
            },
            {
                path: 'specificationsConfiguring',
                component: Configuring,
            },

            // Development
            {
                path: 'developmentChangelog',
                component: Changelog,
            },
            {
                path: 'developmentContributing',
                component: Contributing,
            }
        ]},
    {path:'/about', component: AboutView},
    {path:'/news', component: NewsView},
]

const router = createRouter({
    history: createMemoryHistory(),
    routes,
})

createApp(App)
    .use(router)
    .mount('#app')
