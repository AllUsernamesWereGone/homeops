import {Component, EventEmitter, Input, Output} from '@angular/core';
import {RouterLink, RouterLinkActive} from '@angular/router';

@Component({
  selector: 'app-sidebar',
  standalone: true,
  imports: [
    RouterLink,
    RouterLinkActive,
  ],
  templateUrl: './sidebar-left.component.html',
  styleUrl: './sidebar-left.component.scss',
})
export class SidebarLeftComponent {
  @Input() collapsed = false;

  @Output() toggleSidebar = new EventEmitter<void>();
}
