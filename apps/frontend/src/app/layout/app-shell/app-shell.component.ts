import {Component, inject, signal} from '@angular/core';
import {ActivatedRouteSnapshot, NavigationEnd, Router, RouterOutlet,} from '@angular/router';
import {toSignal} from '@angular/core/rxjs-interop';
import {filter, map, startWith} from 'rxjs';

import {HeaderComponent} from './header/header.component';
import {SidebarLeftComponent} from './body/sidebar/sidebar-left/sidebar-left.component';
import {FooterComponent} from './footer/footer.component';
import {SidebarRightComponent} from "./body/sidebar/sidebar-right/sidebar-right.component";

@Component({
    selector: 'app-shell',
    standalone: true,
    imports: [
        RouterOutlet,
        HeaderComponent,
        SidebarLeftComponent,
        FooterComponent,
        SidebarRightComponent
    ],
    templateUrl: './app-shell.component.html',
    styleUrl: './app-shell.component.scss',
})
export class AppShellComponent {
    private readonly router = inject(Router);

    protected readonly sidebarCollapsed = signal(false);

    protected readonly showRightSidebar = toSignal(
        this.router.events.pipe(
            filter((event): event is NavigationEnd => event instanceof NavigationEnd),
            startWith(null),
            map(() => this.hasRightSidebar(this.router.routerState.snapshot.root)),
        ),
        {initialValue: false},
    );

    protected toggleSidebar(): void {
        this.sidebarCollapsed.update(collapsed => !collapsed);
    }

    private hasRightSidebar(route: ActivatedRouteSnapshot | null | undefined): boolean {
        let currentRoute = route;

        while (currentRoute) {
            if (currentRoute.data?.['rightSidebar'] === true) {
                return true;
            }

            currentRoute = currentRoute.firstChild ?? null;
        }

        return false;
    }
}
